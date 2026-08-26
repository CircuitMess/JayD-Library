#include "SourceAAC.h"
#include "../PerfMon.h"
#include <limits.h>

#define AAC_READ_BUFFER 1024 * 64
#define AAC_READ_CHUNK 1024 * 4 // should be bigger than min input
#define AAC_DECODE_BUFFER 8192
#define AAC_OUT_BUFFER 32768
#define AAC_MAX_DECODED_BYTES 8192
#define AAC_INDEX_PSRAM_ENTRIES 16384
#define AAC_INDEX_INTERNAL_ENTRIES 2048
#define AAC_INDEX_READ_CHUNK 4096

SourceAAC::SourceAAC() :
		readBuffer(AAC_READ_BUFFER),
		fillBuffer(AAC_DECODE_BUFFER),
		dataBuffer(AAC_OUT_BUFFER){

}

SourceAAC::SourceAAC(fs::File file) : SourceAAC(){
	open(file);
}

void SourceAAC::open(fs::File file){
	close();

	this->file = file;
	channels = sampleRate = bytesPerSample = 0;
	readBuffer.clear();
	dataBuffer.clear();
	fillBuffer.clear();

	if(!file){
		return;
	}

	bytesPerSample = 2;
	buildFrameIndex();
	if(sourceSampleRate == 0){
		Serial.println("SourceAAC: no valid ADTS frames");
		status = Status::FAILED;
		return;
	}
	file.seek(firstFrameOffset);

	hAACDecoder = AACInitDecoder();
	if(hAACDecoder == nullptr){
		Serial.println("Decoder construct fail");
		status = Status::FAILED;
		return;
	}

	status = Status::STARVED;
	addReadJob(true);
}

void SourceAAC::setSongDoneCallback(void (*callback)()) {
	songDoneCallback = callback;
}

bool SourceAAC::isReadReady() const {
	return !readJobPending || readResult != nullptr;
}

SourceAAC::Status SourceAAC::getStatus() const {
	return status.load();
}

void SourceAAC::close(){
	if(readJobPending){
		while(readResult == nullptr){
			Sched.loop(0);
		}

		free(readResult->buffer);
		delete readResult;
		readResult = nullptr;
		readJobPending = false;
	}

	channels = sampleRate = bytesPerSample = 0;
	readBuffer.clear();
	dataBuffer.clear();
	fillBuffer.clear();

	if(hAACDecoder){
		AACFreeDecoder(hAACDecoder);
		hAACDecoder = nullptr;
	}
	freeFrameIndex();
	durationSourceFrames = 0;
	indexedSourceFrameEnd = 0;
	portENTER_CRITICAL(&timingMux);
	elapsedSourceFrames = 0;
	portEXIT_CRITICAL(&timingMux);
	decodedSourceFrame = seekTargetSourceFrame = elapsedFrameRemainder = 0;
	sourceSampleRate = firstFrameOffset = 0;
	sourceChannels = adtsChannelConfiguration = 0;
	readEof = false;
	discardPendingRead = false;
	eofNotification.reset();
	status = Status::CLOSED;
}

SourceAAC::~SourceAAC(){
	SourceAAC::close();
}

void SourceAAC::addReadJob(bool full){
	if(readJobPending || readEof || !file) return;

	delete readResult;
	readResult = nullptr;

	size_t size = full ? readBuffer.writeAvailable() : AAC_READ_CHUNK;

	//Serial.printf("Adding read job, size: %ld\n", size);

	if(size == 0 || readBuffer.writeAvailable() < size){
		return;
	}

	uint8_t* buf = nullptr;
	if(size <= AAC_READ_CHUNK || !psramFound()){
		buf = static_cast<uint8_t*>(malloc(size));
	}else{
		buf = static_cast<uint8_t*>(ps_malloc(size));
	}
	if(buf == nullptr){
		Serial.println("SourceAAC: read buffer allocation failed");
		readEof = true;
		return;
	}

	Sched.addJob(new SDJob{
						 .type = SDJob::SD_READ,
						 .file = file,
						 .size = size,
						 .buffer = buf,
						 .result = &readResult
				 });

	readJobPending = true;
}

void SourceAAC::processReadJob(bool wait){
	if(!readJobPending) return;
	if(readResult == nullptr){
		if(wait || readBuffer.readAvailable() + fillBuffer.readAvailable() < 7){
			while(readResult == nullptr){
				delayMicroseconds(1);
			}
		}else{
			return;
		}
	}

	readBuffer.write(readResult->buffer, readResult->size);
	if(readResult->size == 0) readEof = true;
	free(readResult->buffer);

	delete readResult;
	readResult = nullptr;

	readJobPending = false;
	if(discardPendingRead){
		discardPendingRead = false;
		readBuffer.clear();
		readEof = false;
		addReadJob();
		if(wait) processReadJob(true);
	}
}

bool SourceAAC::prepareNextFrame(ADTSTiming::Header& header){
	for(;;){
		refill();
		const size_t available = fillBuffer.readAvailable();
		const uint8_t* data = fillBuffer.readData();
		bool waitingForData = false;

		for(size_t offset = 0; offset + 7 <= available; offset++){
			const ADTSTiming::ParseResult result = ADTSTiming::parseHeader(data + offset, available - offset, header);
			if(result == ADTSTiming::INVALID) continue;
			if(result == ADTSTiming::VALID &&
			   header.sampleRate == sourceSampleRate &&
			   (adtsChannelConfiguration == 0 || header.channels == adtsChannelConfiguration)){
				if(header.frameLength <= available - offset){
					fillBuffer.readMove(offset);
					return true;
				}
				if(!readEof){
					fillBuffer.readMove(offset);
					addReadJob();
					processReadJob(true);
					waitingForData = true;
					break;
				}
			}
			if(result == ADTSTiming::NEED_MORE && !readEof){
				fillBuffer.readMove(offset);
				addReadJob();
				processReadJob(true);
				waitingForData = true;
				break;
			}
		}
		if(waitingForData) continue;

		if(readEof){
			fillBuffer.readMove(fillBuffer.readAvailable());
			return false;
		}

		const size_t keep = min(size_t(8), fillBuffer.readAvailable());
		fillBuffer.readMove(fillBuffer.readAvailable() - keep);
		addReadJob();
		processReadJob(true);
	}
}

size_t SourceAAC::generate(int16_t* outBuffer){
	if(!file){
		Serial.println("file false");
		status = Status::FAILED;
		return 0;
	}

	if(!hAACDecoder){
		Serial.println("Decoder false");
		status = Status::FAILED;
		return 0;
	}

	Profiler.start("AAC read job process");
	processReadJob();
	Profiler.end();

	Profiler.start("AAC decode");
	while(dataBuffer.readAvailable() < BUFFER_SIZE){
		ADTSTiming::Header adts;
		if(!prepareNextFrame(adts)) break;
		const size_t rawBlocks = adts.sourceFrames / 1024;
		size_t requiredBytes = 0;
		if(!ADTSTiming::requiredDecodeBytes(
				rawBlocks, AAC_MAX_DECODED_BYTES,
				AAC_OUT_BUFFER, requiredBytes) ||
		   dataBuffer.writeAvailable() < requiredBytes){
			Serial.println("SourceAAC: decoded frame exceeds output buffer");
			break;
		}

		uint8_t* data = const_cast<uint8_t*>(fillBuffer.readData());
		int bytesLeft = adts.frameLength;
		size_t decodedBlocks = 0;
		for(; decodedBlocks < rawBlocks; decodedBlocks++){
			if(dataBuffer.writeAvailable() < AAC_MAX_DECODED_BYTES){
				Serial.println("SourceAAC: insufficient decoder output space");
				AACFlushCodec(hAACDecoder);
				break;
			}
			int16_t* pcm = reinterpret_cast<int16_t*>(dataBuffer.writeData());
			const int ret = AACDecode(hAACDecoder, &data, &bytesLeft, pcm);
			if(ret){
				Serial.printf("decode error %d, frame size %u B\n", ret, adts.frameLength);
				AACFlushCodec(hAACDecoder);
				break;
			}

			AACFrameInfo fi;
			AACGetLastFrameInfo(hAACDecoder, &fi);
			sampleRate = fi.sampRateOut;
			channels = fi.nChans;
			if(sampleRate == 0 || channels == 0 ||
			   fi.outputSamps <= 0 || fi.outputSamps % channels != 0){
				Serial.println("SourceAAC: invalid decoder frame info");
				AACFlushCodec(hAACDecoder);
				break;
			}
			if(sourceChannels == 0) sourceChannels = channels;

			const size_t outputFrames = fi.outputSamps / channels;
			if(channels > 1){
				for(size_t frame = 0; frame < outputFrames; frame++){
					int32_t mixed = 0;
					for(uint8_t channel = 0; channel < channels; channel++){
						mixed += pcm[frame * channels + channel];
					}
					pcm[frame] = int16_t(mixed / channels);
				}
			}

			size_t discardedFrames = 0;
			if(seekTargetSourceFrame > decodedSourceFrame){
				const uint64_t sourceFramesToDiscard = min(
						uint64_t(1024),
						seekTargetSourceFrame - decodedSourceFrame);
				discardedFrames = size_t((sourceFramesToDiscard * outputFrames + 1023) / 1024);
				discardedFrames = min(discardedFrames, outputFrames);
				memmove(pcm, pcm + discardedFrames, (outputFrames - discardedFrames) * bytesPerSample);
			}
			decodedSourceFrame += 1024;
			if(decodedSourceFrame >= seekTargetSourceFrame) seekTargetSourceFrame = 0;

			dataBuffer.writeMove((outputFrames - discardedFrames) * bytesPerSample);
		}
		decodedSourceFrame += (rawBlocks - decodedBlocks) * 1024;
		fillBuffer.readMove(adts.frameLength);
	}
	Profiler.end();

	size_t size = min((size_t) BUFFER_SIZE, dataBuffer.readAvailable());
	memcpy(outBuffer, dataBuffer.readData(), size);
	dataBuffer.readMove(size);
	size_t samples = size / (NUM_CHANNELS * BYTES_PER_SAMPLE);

	for(int i = 0; i < samples; i++){
		outBuffer[i] *= volume;
	}

	if(samples == 0){
		status = Status::END_OF_STREAM;
		if(readEof && eofNotification.take() && songDoneCallback != nullptr) {
			songDoneCallback();
		}
		if(repeat && !rewindAttempted){
			rewindAttempted = true;
			if(seekSourceFrame(0)){
				const size_t repeatedSamples = generate(outBuffer);
				rewindAttempted = false;
				return repeatedSamples;
			}
			rewindAttempted = false;
		}
	}else{
		status = Status::DATA;
		const uint32_t outputRate = sampleRate == 0 ? sourceSampleRate : sampleRate;
		const uint64_t numerator = elapsedFrameRemainder + uint64_t(samples) * sourceSampleRate;
		const uint64_t advanced = numerator / outputRate;
		elapsedFrameRemainder = numerator % outputRate;
		portENTER_CRITICAL(&timingMux);
		elapsedSourceFrames = min(durationSourceFrames, elapsedSourceFrames + advanced);
		portEXIT_CRITICAL(&timingMux);
		rewindAttempted = false;
		addReadJob();
	}

	return samples;
}

void SourceAAC::refill(){
	size_t size = min(fillBuffer.writeAvailable(), readBuffer.readAvailable());
	size = readBuffer.read(fillBuffer.writeData(), size);
	fillBuffer.writeMove(size);
}

int SourceAAC::available(){
	const uint64_t elapsed = getElapsedSourceFrames();
	const uint64_t remaining = elapsed < durationSourceFrames ? durationSourceFrames - elapsed : 0;
	return remaining > INT_MAX ? INT_MAX : int(remaining);
}

uint16_t SourceAAC::getDuration(){
	if(sourceSampleRate == 0) return 0;
	const uint64_t seconds = durationSourceFrames / sourceSampleRate;
	return seconds > UINT16_MAX ? UINT16_MAX : uint16_t(seconds);
}

uint16_t SourceAAC::getElapsed(){
	if(sourceSampleRate == 0) return 0;
	const uint64_t seconds = getElapsedSourceFrames() / sourceSampleRate;
	return seconds > UINT16_MAX ? UINT16_MAX : uint16_t(seconds);
}

void SourceAAC::seek(uint16_t time, fs::SeekMode mode){
	uint64_t frames;
	if(!ADTSTiming::secondsToFrames(time, sourceSampleRate, frames)) return;
	if(mode == SeekCur){
		const uint64_t elapsed = getElapsedSourceFrames();
		frames = frames > UINT64_MAX - elapsed ? UINT64_MAX : frames + elapsed;
	}else if(mode == SeekEnd){
		frames = frames >= durationSourceFrames ? 0 : durationSourceFrames - frames;
	}
	seekSourceFrame(frames);
}

bool SourceAAC::seekSourceFrame(uint64_t frame){
	if(sourceSampleRate == 0 || frame > UINT32_MAX) return false;
	frame = min(frame, durationSourceFrames);
	uint32_t offset = firstFrameOffset;
	uint64_t indexedFrame = 0;
	if(frame != 0){
		if(frameIndexCount == 0) return false;
		const size_t index = ADTSTiming::findPreceding(frameIndex, frameIndexCount, frame);
		if(frameIndex[index].sourceFrame > frame ||
		   (frameIndexQuality != INDEX_COMPLETE && frame > indexedSourceFrameEnd)){
			return false;
		}
		offset = frameIndex[index].offset;
		indexedFrame = frameIndex[index].sourceFrame;
	}
	if(readJobPending && readResult != nullptr){
		free(readResult->buffer);
		delete readResult;
		readResult = nullptr;
		readJobPending = false;
		discardPendingRead = false;
	}else if(readJobPending){
		discardPendingRead = true;
	}
	Sched.addJob(new SDJob{
			.type = SDJob::SD_SEEK,
			.file = file,
			.size = offset,
			.buffer = nullptr,
			.result = nullptr
	});
	portENTER_CRITICAL(&timingMux);
	elapsedSourceFrames = frame;
	portEXIT_CRITICAL(&timingMux);
	decodedSourceFrame = indexedFrame;
	seekTargetSourceFrame = frame;
	elapsedFrameRemainder = 0;
	eofNotification.reset();
	resetDecoding();
	return true;
}

void SourceAAC::setVolume(uint8_t volume){
	SourceAAC::volume = (float) volume / 255.0f;
}

void SourceAAC::resetDecoding() {
	readBuffer.clear();
	dataBuffer.clear();
	fillBuffer.clear();
	AACFlushCodec(hAACDecoder);
	readEof = false;

	addReadJob();
}

void SourceAAC::setRepeat(bool repeat) {
	SourceAAC::repeat = repeat;
}

uint64_t SourceAAC::getDurationSourceFrames() const {
	return durationSourceFrames;
}

uint64_t SourceAAC::getElapsedSourceFrames() const {
	portENTER_CRITICAL(&timingMux);
	const uint64_t frames = elapsedSourceFrames;
	portEXIT_CRITICAL(&timingMux);
	return frames;
}

uint32_t SourceAAC::getSourceSampleRate() const {
	return sourceSampleRate;
}

uint8_t SourceAAC::getSourceChannels() const {
	return sourceChannels;
}

SourceAAC::FrameIndexQuality SourceAAC::getFrameIndexQuality() const {
	return frameIndexQuality;
}

void SourceAAC::freeFrameIndex(){
	free(frameIndex);
	frameIndex = nullptr;
	frameIndexCount = frameIndexCapacity = 0;
	frameIndexQuality = INDEX_UNAVAILABLE;
}

void SourceAAC::buildFrameIndex(){
	freeFrameIndex();
	durationSourceFrames = 0;
	indexedSourceFrameEnd = 0;
	sourceSampleRate = 0;
	sourceChannels = adtsChannelConfiguration = 0;
	firstFrameOffset = 0;

	const size_t fileSize = file.size();
	if(fileSize < 7) return;

	frameIndexCapacity = min(
			fileSize / size_t(7),
			size_t(psramFound() ? AAC_INDEX_PSRAM_ENTRIES : AAC_INDEX_INTERNAL_ENTRIES));
	if(frameIndexCapacity > 0){
		const size_t bytes = frameIndexCapacity * sizeof(ADTSTiming::FrameIndexEntry);
		frameIndex = static_cast<ADTSTiming::FrameIndexEntry*>(
				psramFound() ? ps_malloc(bytes) : malloc(bytes));
		if(frameIndex == nullptr) frameIndexCapacity = 0;
	}

	uint8_t* cache = static_cast<uint8_t*>(
			psramFound() ? ps_malloc(AAC_INDEX_READ_CHUNK) : malloc(AAC_INDEX_READ_CHUNK));
	if(cache == nullptr){
		Serial.println("SourceAAC: ADTS scan buffer allocation failed");
		return;
	}

	size_t cacheStart = fileSize;
	size_t cacheSize = 0;
	auto readAt = [&](size_t position, uint8_t* out, size_t count) -> bool {
		size_t copied = 0;
		while(copied < count){
			if(position < cacheStart || position >= cacheStart + cacheSize){
				if(!file.seek(position)) return false;
				cacheStart = position;
				cacheSize = file.read(cache, min(size_t(AAC_INDEX_READ_CHUNK), fileSize - position));
				Sched.loop(0);
				if(cacheSize == 0) return false;
			}
			const size_t cacheOffset = position - cacheStart;
			const size_t part = min(count - copied, cacheSize - cacheOffset);
			memcpy(out + copied, cache + cacheOffset, part);
			position += part;
			copied += part;
		}
		return true;
	};

	uint8_t bytes[9];
	size_t offset = 0;
	bool foundFrame = false;
	bool indexFull = false;
	bool scanFailed = false;
	while(offset + 7 <= fileSize){
		if(!readAt(offset, bytes, 7)){
			scanFailed = true;
			break;
		}
		ADTSTiming::Header header;
		ADTSTiming::ParseResult result = ADTSTiming::parseHeader(bytes, 7, header);
		if(result == ADTSTiming::NEED_MORE && offset + 9 <= fileSize){
			if(!readAt(offset, bytes, 9)){
				scanFailed = true;
				break;
			}
			result = ADTSTiming::parseHeader(bytes, 9, header);
		}
		if(result != ADTSTiming::VALID ||
		   header.frameLength > fileSize - offset ||
		   (foundFrame && (header.sampleRate != sourceSampleRate ||
						   (sourceChannels != 0 && header.channels != sourceChannels)))){
			offset++;
			continue;
		}

		if(!foundFrame){
			foundFrame = true;
			firstFrameOffset = uint32_t(offset);
			sourceSampleRate = header.sampleRate;
			sourceChannels = adtsChannelConfiguration = header.channels;
		}
		const bool stored = frameIndexCount < frameIndexCapacity && durationSourceFrames <= UINT32_MAX;
		if(stored){
			frameIndex[frameIndexCount++] = {
					uint32_t(offset),
					uint32_t(durationSourceFrames)
			};
		}else{
			indexFull = true;
		}
		if(durationSourceFrames > UINT64_MAX - header.sourceFrames){
			durationSourceFrames = UINT64_MAX;
			indexFull = true;
			break;
		}
		durationSourceFrames += header.sourceFrames;
		if(stored) indexedSourceFrameEnd = durationSourceFrames;
		offset += header.frameLength;
	}
	free(cache);

	if(frameIndexCount == 0){
		frameIndexQuality = INDEX_UNAVAILABLE;
	}else{
		frameIndexQuality = indexFull || scanFailed ? INDEX_PARTIAL : INDEX_COMPLETE;
	}
	if(scanFailed) Serial.println("SourceAAC: ADTS index scan read failed");
}
