#include "SourceAAC.h"
#include "../PerfMon.h"
#include <SD.h>
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
		dataBuffer(AAC_OUT_BUFFER),
		indexTask("AACIndex", indexThread, 3 * 1024, this){

}

SourceAAC::SourceAAC(fs::File file) : SourceAAC(){
	open(file);
}

void SourceAAC::open(fs::File file){
	close();

	this->file = file;
	filePath = file ? file.name() : "";
	channels = sampleRate = bytesPerSample = 0;
	readBuffer.clear();
	dataBuffer.clear();
	fillBuffer.clear();

	if(!file){
		return;
	}

	bytesPerSample = 2;
	hAACDecoder = AACInitDecoder();
	if(hAACDecoder == nullptr){
		Serial.println("Decoder construct fail");
		return;
	}

	addReadJob(true);
	startFrameIndex();
}

void SourceAAC::setSongDoneCallback(void (*callback)()) {
	songDoneCallback = callback;
}

bool SourceAAC::isReadReady() const {
	const bool initialReadReady = !readJobPending || readResult != nullptr;
	return ADTSTiming::playbackReady(
			initialReadReady, getFrameIndexQuality() != INDEX_PENDING);
}

void SourceAAC::close(){
	if(!indexTask.isStopped()){
		indexTask.stop();
		while(!indexTask.isStopped()){
			Sched.loop(0);
			delay(1);
		}
	}
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
	indexMutex.lock();
	durationSourceFrames = 0;
	indexedSourceFrameEnd = 0;
	sourceSampleRate = firstFrameOffset = 0;
	sourceChannels = adtsChannelConfiguration = 0;
	indexMutex.unlock();
	portENTER_CRITICAL(&timingMux);
	elapsedSourceFrames = 0;
	portEXIT_CRITICAL(&timingMux);
	decodedSourceFrame = seekTargetSourceFrame = elapsedFrameRemainder = 0;
	filePath = "";
	readEof = false;
	discardPendingRead = false;
	eofNotification.reset();
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
			indexMutex.lock();
			const bool firstFrame = sourceSampleRate == 0;
			const bool compatible = firstFrame ||
					(header.sampleRate == sourceSampleRate &&
					 (adtsChannelConfiguration == 0 ||
					  header.channels == adtsChannelConfiguration));
			if(firstFrame && result == ADTSTiming::VALID){
				sourceSampleRate = header.sampleRate;
				sourceChannels = adtsChannelConfiguration = header.channels;
			}
			indexMutex.unlock();
			if(result == ADTSTiming::VALID && compatible){
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
		return 0;
	}

	if(!hAACDecoder){
		Serial.println("Decoder false");
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
		indexMutex.lock();
		const uint32_t sourceRate = sourceSampleRate;
		const uint64_t duration = durationSourceFrames;
		const bool durationKnown = frameIndexQuality == INDEX_COMPLETE ||
								   frameIndexQuality == INDEX_PARTIAL;
		indexMutex.unlock();
		const uint32_t outputRate = sampleRate == 0 ? sourceRate : sampleRate;
		const uint64_t numerator = elapsedFrameRemainder + uint64_t(samples) * sourceRate;
		const uint64_t advanced = numerator / outputRate;
		elapsedFrameRemainder = numerator % outputRate;
		portENTER_CRITICAL(&timingMux);
		elapsedSourceFrames = durationKnown
							  ? min(duration, elapsedSourceFrames + advanced)
							  : elapsedSourceFrames + advanced;
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
	const uint64_t duration = getDurationSourceFrames();
	const uint64_t remaining = elapsed < duration ? duration - elapsed : 0;
	return remaining > INT_MAX ? INT_MAX : int(remaining);
}

uint16_t SourceAAC::getDuration(){
	indexMutex.lock();
	const uint32_t rate = sourceSampleRate;
	const uint64_t duration = durationSourceFrames;
	const bool ready = frameIndexQuality == INDEX_COMPLETE ||
					   frameIndexQuality == INDEX_PARTIAL;
	indexMutex.unlock();
	if(!ready || rate == 0) return 0;
	const uint64_t seconds = duration / rate;
	return seconds > UINT16_MAX ? UINT16_MAX : uint16_t(seconds);
}

uint16_t SourceAAC::getElapsed(){
	const uint32_t rate = getSourceSampleRate();
	if(rate == 0) return 0;
	const uint64_t seconds = getElapsedSourceFrames() / rate;
	return seconds > UINT16_MAX ? UINT16_MAX : uint16_t(seconds);
}

void SourceAAC::seek(uint16_t time, fs::SeekMode mode){
	uint64_t frames;
	if(!ADTSTiming::secondsToFrames(time, getSourceSampleRate(), frames)) return;
	if(mode == SeekCur){
		const uint64_t elapsed = getElapsedSourceFrames();
		frames = frames > UINT64_MAX - elapsed ? UINT64_MAX : frames + elapsed;
	}else if(mode == SeekEnd){
		const uint64_t duration = getDurationSourceFrames();
		if(duration == 0 && frames != 0){
			seekSourceFrame(frames);
			return;
		}
		frames = frames >= duration ? 0 : duration - frames;
	}
	seekSourceFrame(frames);
}

bool SourceAAC::seekSourceFrame(uint64_t frame){
	if(frame > UINT32_MAX) return false;
	uint32_t offset = 0;
	uint64_t indexedFrame = 0;
	indexMutex.lock();
	offset = firstFrameOffset;
	const FrameIndexQuality quality = frameIndexQuality;
	const uint64_t duration = durationSourceFrames;
	const uint64_t indexedEnd = indexedSourceFrameEnd;
	const size_t indexCount = frameIndexCount;
	if(sourceSampleRate == 0 || ((quality == INDEX_PENDING ||
								quality == INDEX_UNAVAILABLE) && frame != 0)){
		indexMutex.unlock();
		return false;
	}
	frame = quality == INDEX_PENDING || quality == INDEX_UNAVAILABLE
			? 0 : min(frame, duration);
	if(frame != 0){
		if(indexCount == 0){
			indexMutex.unlock();
			return false;
		}
		const size_t index = ADTSTiming::findPreceding(frameIndex, indexCount, frame);
		if(frameIndex[index].sourceFrame > frame ||
		   (quality != INDEX_COMPLETE && frame > indexedEnd)){
			indexMutex.unlock();
			return false;
		}
		offset = frameIndex[index].offset;
		indexedFrame = frameIndex[index].sourceFrame;
	}
	indexMutex.unlock();
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
	indexMutex.lock();
	const uint64_t frames = frameIndexQuality == INDEX_COMPLETE ||
							frameIndexQuality == INDEX_PARTIAL
							? durationSourceFrames : 0;
	indexMutex.unlock();
	return frames;
}

uint64_t SourceAAC::getElapsedSourceFrames() const {
	portENTER_CRITICAL(&timingMux);
	const uint64_t frames = elapsedSourceFrames;
	portEXIT_CRITICAL(&timingMux);
	return frames;
}

uint32_t SourceAAC::getSourceSampleRate() const {
	indexMutex.lock();
	const uint32_t rate = sourceSampleRate;
	indexMutex.unlock();
	return rate;
}

uint8_t SourceAAC::getSourceChannels() const {
	indexMutex.lock();
	const uint8_t count = sourceChannels;
	indexMutex.unlock();
	return count;
}

SourceAAC::FrameIndexQuality SourceAAC::getFrameIndexQuality() const {
	indexMutex.lock();
	const FrameIndexQuality quality = frameIndexQuality;
	indexMutex.unlock();
	return quality;
}

void SourceAAC::freeFrameIndex(){
	indexMutex.lock();
	free(frameIndex);
	frameIndex = nullptr;
	frameIndexCount = frameIndexCapacity = 0;
	frameIndexQuality = INDEX_UNAVAILABLE;
	indexMutex.unlock();
}

void SourceAAC::startFrameIndex(){
	indexMutex.lock();
	frameIndexQuality = INDEX_PENDING;
	indexMutex.unlock();
	indexTask.start(0);
}

void SourceAAC::indexThread(Task* task){
	static_cast<SourceAAC*>(task->arg)->buildFrameIndex(task);
}

void SourceAAC::buildFrameIndex(Task* task){
	fs::File indexFile = SD.open(filePath);
	const size_t fileSize = indexFile ? indexFile.size() : 0;
	size_t indexCapacity = min(
			fileSize / size_t(7),
			size_t(psramFound() ? AAC_INDEX_PSRAM_ENTRIES : AAC_INDEX_INTERNAL_ENTRIES));
	ADTSTiming::FrameIndexEntry* newIndex = nullptr;
	if(indexCapacity > 0){
		const size_t bytes = indexCapacity * sizeof(ADTSTiming::FrameIndexEntry);
		newIndex = static_cast<ADTSTiming::FrameIndexEntry*>(
				psramFound() ? ps_malloc(bytes) : malloc(bytes));
	}
	const bool indexAllocationFailed = indexCapacity > 0 && newIndex == nullptr;
	if(indexAllocationFailed){
		Serial.println("SourceAAC: ADTS seek index allocation failed; duration only");
		indexCapacity = 0;
	}

	uint8_t* cache = static_cast<uint8_t*>(
			psramFound() ? ps_malloc(AAC_INDEX_READ_CHUNK) : malloc(AAC_INDEX_READ_CHUNK));
	if(!indexFile || fileSize < 7 || cache == nullptr){
		Serial.println("SourceAAC: ADTS scan buffer allocation failed");
		free(cache);
		free(newIndex);
		indexMutex.lock();
		frameIndexQuality = INDEX_UNAVAILABLE;
		indexMutex.unlock();
		return;
	}

	size_t cacheStart = fileSize;
	size_t cacheSize = 0;
	auto readAt = [&](size_t position, uint8_t* out, size_t count) -> bool {
		size_t copied = 0;
		while(copied < count){
			if(position < cacheStart || position >= cacheStart + cacheSize){
				SDResult* seekResult = nullptr;
				Sched.addJob(new SDJob{
						.type = SDJob::SD_SEEK,
						.file = indexFile,
						.size = position,
						.buffer = nullptr,
						.result = &seekResult
				});
				while(seekResult == nullptr) delay(1);
				const bool seeked = position == 0 || seekResult->size == position;
				delete seekResult;
				if(!seeked || !task->running) return false;

				SDResult* result = nullptr;
				Sched.addJob(new SDJob{
						.type = SDJob::SD_READ,
						.file = indexFile,
						.size = min(size_t(AAC_INDEX_READ_CHUNK), fileSize - position),
						.buffer = cache,
						.result = &result
				});
				while(result == nullptr) delay(1);
				cacheStart = position;
				cacheSize = result->size;
				delete result;
				if(cacheSize == 0) return false;
				delay(1);
				if(!task->running) return false;
			}
			const size_t cacheOffset = position - cacheStart;
			const size_t part = min(count - copied, cacheSize - cacheOffset);
			memcpy(out + copied, cache + cacheOffset, part);
			position += part;
			copied += part;
		}
		return true;
	};

	size_t newIndexCount = 0;
	uint64_t newIndexedEnd = 0;
	bool indexFull = indexAllocationFailed;
	ADTSTiming::ScanSummary summary;
	const ADTSTiming::ScanResult scanResult = ADTSTiming::scanFrames(
			fileSize,
			readAt,
			[&](uint32_t offset, uint64_t sourceFrame, const ADTSTiming::Header& header){
				const bool stored = newIndexCount < indexCapacity && sourceFrame <= UINT32_MAX;
				if(stored){
					newIndex[newIndexCount++] = {
							offset,
							uint32_t(sourceFrame)
					};
					newIndexedEnd = sourceFrame + header.sourceFrames;
				}else{
					indexFull = true;
				}
			},
			[&](){ return task->running; },
			summary);
	free(cache);
	indexFile.close();
	if(scanResult == ADTSTiming::SCAN_CANCELLED || !task->running){
		free(newIndex);
		return;
	}
	const bool scanFailed = scanResult == ADTSTiming::SCAN_READ_ERROR;
	if(summary.overflow) indexFull = true;

	indexMutex.lock();
	free(frameIndex);
	frameIndex = newIndex;
	frameIndexCount = newIndexCount;
	frameIndexCapacity = indexCapacity;
	durationSourceFrames = summary.sourceFrames;
	indexedSourceFrameEnd = newIndexedEnd;
	firstFrameOffset = summary.firstFrameOffset;
	if(sourceSampleRate == 0) sourceSampleRate = summary.sampleRate;
	if(sourceChannels == 0) sourceChannels = summary.channels;
	if(adtsChannelConfiguration == 0) adtsChannelConfiguration = summary.channels;
	frameIndexQuality = summary.sampleRate == 0
						? INDEX_UNAVAILABLE
						: (indexFull || scanFailed ? INDEX_PARTIAL : INDEX_COMPLETE);
	portENTER_CRITICAL(&timingMux);
	elapsedSourceFrames = min(durationSourceFrames, elapsedSourceFrames);
	portEXIT_CRITICAL(&timingMux);
	indexMutex.unlock();
	if(scanFailed) Serial.println("SourceAAC: ADTS index scan read failed");
}
