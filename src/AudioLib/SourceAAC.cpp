#include "SourceAAC.h"
#include "../PerfMon.h"

#define AAC_READ_BUFFER 1024 * 64
#define AAC_READ_CHUNK 1024 * 4 // should be bigger than min input
#define AAC_DECODE_MIN_INPUT 1024 // should be smaller than read chunk
#define AAC_OUT_BUFFER 1024 * 4

SourceAAC::SourceAAC() :
		readBuffer(AAC_READ_BUFFER),
		fillBuffer(AAC_DECODE_MIN_INPUT),
		dataBuffer(AAC_OUT_BUFFER){

}

SourceAAC::SourceAAC(fs::File file) : SourceAAC(){
	open(file);
}

void SourceAAC::open(fs::File file){
	this->file = file;
	channels = sampleRate = bytesPerSample = bitrate = readData = 0;
	readBuffer.clear();
	dataBuffer.clear();

	if(!file){
		return;
	}

	dataSize = file.size();
	bitrate = 128000;

	hAACDecoder = AACInitDecoder();
	if(hAACDecoder == nullptr){
		Serial.println("Decoder construct fail");

	}else{
		Serial.println("Decoder constructed");
	}

	addReadJob(true);
}

SourceAAC::~SourceAAC(){

}

void SourceAAC::addReadJob(bool full){
	if(readJobPending) return;

	delete readResult;
	readResult = nullptr;

	size_t size = full ? readBuffer.writeAvailable() : AAC_READ_CHUNK;

	//Serial.printf("Adding read job, size: %ld\n", size);

	if(size == 0 || readBuffer.writeAvailable() < size){
		return;
	}

	uint8_t* buf;
	if(size <= AAC_READ_CHUNK || !psramFound()){
		buf = static_cast<uint8_t*>(malloc(size));
	}else{
		buf = static_cast<uint8_t*>(ps_malloc(size));
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

void SourceAAC::processReadJob(){
	if(readResult == nullptr){
		if(readBuffer.readAvailable() + fillBuffer.readAvailable() < AAC_DECODE_MIN_INPUT){
			Serial.println("small");
			while(readResult == nullptr){
				delayMicroseconds(1);
			}
			Serial.println("ok");
		}else{
			return;
		}
	}

	readBuffer.write(readResult->buffer, readResult->size);
	free(readResult->buffer);

	delete readResult;
	readResult = nullptr;

	readJobPending = false;
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

	Profiler.start("decode");
	while(dataBuffer.readAvailable() < BUFFER_SIZE){
		// Serial.printf("Grabbing, available %ld, taking %ld\n", readBuffer.readAvailable(), fillBuffer.writeAvailable());
		//Profiler.start("fill read");
		fillBuffer.writeMove(readBuffer.read(fillBuffer.writeData(), fillBuffer.writeAvailable()));
		if(fillBuffer.readAvailable() < AAC_DECODE_MIN_INPUT){
			Serial.println("rezerva");
			addReadJob();
			processReadJob();
			fillBuffer.writeMove(readBuffer.read(fillBuffer.writeData(), fillBuffer.writeAvailable()));
		}
		//Profiler.end();

		ADTSHeader* adts = (ADTSHeader*) fillBuffer.readData();
		if(adts->syncword_0_to_8 != 0xff || adts->syncword_9_to_12 != 0xf){
			Serial.println("Incorrect frame, searching...");

			size_t bytesMoved = 0;
			while((adts->syncword_0_to_8 != 0xff || adts->syncword_9_to_12 != 0xf) && (++bytesMoved + sizeof(ADTSHeader)) < fillBuffer.readAvailable()){
				adts = (ADTSHeader*) (fillBuffer.readData() + bytesMoved);
			}

			if(adts->syncword_0_to_8 != 0xff || adts->syncword_9_to_12 != 0xf && (bytesMoved + sizeof(ADTSHeader)) == fillBuffer.readAvailable()){
				Serial.printf("Can't find frame. searched %lu bytes\n", bytesMoved);
				return 0;
			}else{
				Serial.printf("Frame found after %lu bytes\n", bytesMoved);
			}

			fillBuffer.readMove(bytesMoved);
			fillBuffer.writeMove(readBuffer.read(fillBuffer.writeData(), fillBuffer.writeAvailable()));
		}

		uint frameSize = 0; //adts->frame_length_0_to_1 << 11 | adts->frame_length_2_to_9 << 3 | adts->frame_length_10_to_12;

		uint8_t* data = const_cast<uint8_t*>(fillBuffer.readData());
		int bytesLeft = fillBuffer.readAvailable();
		// Serial.printf("Decoding, available %ld\n", fillBuffer.readAvailable());
		int ret = AACDecode(hAACDecoder, &data, &bytesLeft, reinterpret_cast<short*>(dataBuffer.writeData()));
		if(ret){
			Serial.printf("decode error %d, frame size %d B\n", ret, frameSize);
			if(frameSize == 0){
				if(repeat){
					reload();
				}else {
					return 0;
				}
			}else {
				fillBuffer.readMove(frameSize);
			}
			continue;
		}

		fillBuffer.readMove(fillBuffer.readAvailable() - bytesLeft);

		AACFrameInfo fi;
		AACGetLastFrameInfo(hAACDecoder, &fi);

		sampleRate = fi.sampRateOut;
		channels = fi.nChans;
		bytesPerSample = 2;

		dataBuffer.writeMove(fi.outputSamps * channels * bytesPerSample);
	}
	Profiler.end();

	size_t size = min((size_t) BUFFER_SIZE, dataBuffer.readAvailable());
	memcpy(outBuffer, dataBuffer.readData(), size);
	dataBuffer.readMove(size);
	size_t samples = size / (NUM_CHANNELS * BYTES_PER_SAMPLE);

	for(int i = 0; i < samples; i++){
		outBuffer[i] *= volume;
	}

	Profiler.start("AAC read job add");
	addReadJob();
	Profiler.end();

	readData += samples;
	return samples;
}

int SourceAAC::available(){
	if(sampleRate == 0 || channels == 0 || bytesPerSample == 0) return 0;
	return (file.available() / (channels * bytesPerSample));
}

uint16_t SourceAAC::getDuration(){
	if(bitrate == 0) return 0;
	return 8 * dataSize / bitrate;
}

uint16_t SourceAAC::getElapsed(){
	if(bitrate == 0) return 0;
	return readData / SAMPLE_RATE;
}

void SourceAAC::seek(uint16_t time, fs::SeekMode mode){
	if(sampleRate == 0 || channels == 0 || bytesPerSample == 0) return;
	size_t offset = time * sampleRate * channels * bytesPerSample;
	if(offset >= file.size()) return;

	file.seek(offset, mode);
}

void SourceAAC::close(){

}

void SourceAAC::setVolume(uint8_t volume){
	SourceAAC::volume = (float) volume / 255.0f;
}

void SourceAAC::reload() {
	seek(0, SeekSet);
	readBuffer.clear();
	dataBuffer.clear();
	fillBuffer.clear();
	AACFlushCodec(hAACDecoder);
}

void SourceAAC::setRepeat(bool repeat) {
	SourceAAC::repeat = repeat;
}

