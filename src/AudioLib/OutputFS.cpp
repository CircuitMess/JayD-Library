#include "OutputFS.h"
#include <string>
#include "../AudioSetup.hpp"
#include "../PerfMon.h"


struct wavHeader{
	char RIFF[4];
	uint32_t chunkSize;
	char WAVE[4];
	char fmt[3];
	uint32_t fmtSize;
	uint16_t audioFormat;
	uint16_t numChannels;
	uint32_t sampleRate;
	uint32_t byteRate; // == SampleRate * NumChannels * BitsPerSample/8
	uint16_t blockAlign; // == NumChannels * BitsPerSample/8
	uint16_t bitsPerSample;
	char data[4];
	uint32_t dataSize; // == NumSamples * NumChannels * BitsPerSample/8
};

OutputFS::OutputFS() : outBuffers { DataBuffer(OUTFS_BUFSIZE), DataBuffer(OUTFS_BUFSIZE), DataBuffer(OUTFS_BUFSIZE), DataBuffer(OUTFS_BUFSIZE) }{
	freeBuffers.reserve(OUTFS_BUFCOUNT);
	for(int i = 0; i < OUTFS_BUFCOUNT; i++){
		freeBuffers.push_back(i);
	}

	if(psramFound()){
		decodeBuffer = static_cast<uint8_t*>(ps_malloc(OUTFS_DECODE_BUFSIZE));
	}else{
		decodeBuffer = static_cast<uint8_t*>(malloc(OUTFS_DECODE_BUFSIZE));
	}
}

OutputFS::OutputFS(const fs::File& file) : OutputFS(){
	this->file = file;
}

OutputFS::~OutputFS(){
	free(decodeBuffer);
}

const fs::File& OutputFS::getFile() const{
	return file;
}

void OutputFS::setFile(const fs::File& file){
	OutputFS::file = file;
}

static void* inBuffer[] = { inBuffer };
static INT inBufferIds[] = { IN_AUDIO_DATA };
static INT inBufferSize[] = { BUFFER_SAMPLES * NUM_CHANNELS };

static INT inBufferElSize[] = { BYTES_PER_SAMPLE };
static void* outBuffer[] = { nullptr };
static INT outBufferIds[] = { OUT_BITSTREAM_DATA };
static INT outBufferSize[] = { 0 };

static INT outBufferElSize[] = { sizeof(UCHAR) };

void OutputFS::setupBuffers(){
	inBufDesc.numBufs = sizeof(::inBuffer) / sizeof(void*);
	inBufDesc.bufs = (void**) &::inBuffer;
	inBufDesc.bufferIdentifiers = inBufferIds;
	inBufDesc.bufSizes = inBufferSize;
	inBufDesc.bufElSizes = inBufferElSize;

	outBufDesc.numBufs = sizeof(outBuffer) / sizeof(void*);
	outBufDesc.bufs = (void**) &outBuffer;
	outBufDesc.bufferIdentifiers = outBufferIds;
	outBufDesc.bufSizes = outBufferSize;
	outBufDesc.bufElSizes = outBufferElSize;

	inArgs = {
			.numInSamples = BUFFER_SAMPLES,
			.numAncBytes = 0
	};
}

void OutputFS::output(size_t numSamples){
	processWriteJob();

	while(freeBuffers.empty()){
		Serial.println("Waiting for buffers");
		processWriteJob();
	}

	DataBuffer& buffer = outBuffers[freeBuffers.front()];

	Profiler.start("AAC encode");
	// TODO: move these three to constructor
	::inBuffer[0] = this->inBuffer;
	::outBuffer[0] = decodeBuffer;
	::outBufferSize[0] = OUTFS_DECODE_BUFSIZE;
	inArgs.numInSamples = numSamples * NUM_CHANNELS;
	AACENC_OutArgs outArgs;
	int status = aacEncEncode(encoder, &inBufDesc, &outBufDesc, &inArgs, &outArgs);
	if(status != AACENC_OK){
		Serial.printf("ENC: %x\n", status);
	}
	Profiler.end();

	if(outArgs.numOutBytes != 0){
		memcpy(buffer.writeData(), decodeBuffer, outArgs.numOutBytes);
		buffer.writeMove(outArgs.numOutBytes);
	}

	if(buffer.readAvailable() >= OUTFS_WRITESIZE){
		addWriteJob();
	}
}

void OutputFS::init(){
	dataLength = 0;
	if(!file){
		Serial.println("Output file not open");
		return;
	}

	if(aacEncOpen(&encoder, 0x01, 1) != AACENC_OK){
		Serial.println("encoder create error");
		return;
	}

	if(aacEncoder_SetParam(encoder, AACENC_AOT, 2) != AACENC_OK) Serial.println("1 err");
	if(aacEncoder_SetParam(encoder, AACENC_SAMPLERATE, SAMPLE_RATE) != AACENC_OK) Serial.println("2 err");
	if(aacEncoder_SetParam(encoder, AACENC_CHANNELMODE, MODE_1) != AACENC_OK) Serial.println("4 err");
	if(aacEncoder_SetParam(encoder, AACENC_BITRATE,  128000) != AACENC_OK) Serial.println("3 err"); // 140000

	if(aacEncoder_SetParam(encoder, AACENC_TRANSMUX,  2) != AACENC_OK) Serial.println("5 err");
	if(aacEncoder_SetParam(encoder, AACENC_SIGNALING_MODE,  0) != AACENC_OK) Serial.println("6 err");
	if(aacEncoder_SetParam(encoder, AACENC_AFTERBURNER,  0) != AACENC_OK) Serial.println("7 err");

	if(aacEncEncode(encoder, nullptr, nullptr, nullptr, nullptr) != AACENC_OK){
		Serial.println("encoder first run error");
		return;
	}

	AACENC_InfoStruct info = {0};
	if(aacEncInfo(encoder, &info) != AACENC_OK) {
		Serial.println("encoder info error");
		return;
	}

	/*size_t outSize = max(8192, NUM_CHANNELS * 768);
	Serial.printf("FRAME LENGTH: %ld\n", outSize);*/

	setupBuffers();
	file.seek(0);
}

void OutputFS::deinit(){
	if(!freeBuffers.empty() && outBuffers[freeBuffers.front()].readAvailable() > 0){
		addWriteJob();
	}

	inArgs.numInSamples = -1;
	AACENC_OutArgs outArgs;
	int status = aacEncEncode(encoder, nullptr, &outBufDesc, &inArgs, &outArgs);
	if(status != AACENC_OK){
		Serial.printf("ENC flush: %d\n", status);
	}

	if(outArgs.numOutBytes != 0){
		while(freeBuffers.empty()){
			processWriteJob();
		}

		DataBuffer& buffer = outBuffers[freeBuffers.front()];
		memcpy(buffer.writeData(), decodeBuffer, outArgs.numOutBytes);
		buffer.writeMove(outArgs.numOutBytes);

		addWriteJob();
	}

	aacEncClose(&encoder);

	while(freeBuffers.size() != OUTFS_BUFCOUNT){
		processWriteJob();
	}
}

void OutputFS::addWriteJob(){
	if(freeBuffers.empty()) return;
	uint8_t i = freeBuffers.front();

	Sched.addJob(new SDJob{
			 .type = SDJob::SD_WRITE,
			 .file = file,
			 .size = outBuffers[i].readAvailable(),
			 .buffer = const_cast<uint8_t*>(outBuffers[i].readData()),
			 .result = &writeResult[i]
	 });

	freeBuffers.erase(freeBuffers.begin());
	writePending[i] = true;
}

void OutputFS::processWriteJob(){
	for(int i = 0; i < OUTFS_BUFCOUNT; i++){
		if(!writePending[i]) continue;
		if(writeResult[i] == nullptr) continue;

		outBuffers[i].clear();

		delete writeResult[i];
		writeResult[i] = nullptr;

		writePending[i] = false;
		freeBuffers.push_back(i);
	}
}

void OutputFS::writeHeaderWAV(size_t size){
	wavHeader header;
	memcpy(header.RIFF, "RIFF", 4);
	header.chunkSize = size + 36;
	memcpy(header.WAVE, "WAVE", 4);
	memcpy(header.fmt, "fmt ", 4);
	header.fmtSize = 16;
	header.audioFormat = 1; //PCM
	header.numChannels = NUM_CHANNELS; //2 channels
	header.sampleRate = SAMPLE_RATE;
	header.byteRate = SAMPLE_RATE * NUM_CHANNELS * BYTES_PER_SAMPLE;
	header.blockAlign = NUM_CHANNELS * BYTES_PER_SAMPLE;
	header.bitsPerSample = BYTES_PER_SAMPLE * 8;
	memcpy(header.data, "data", 4);
	header.dataSize = size;

	file.seek(0);
	file.write((uint8_t*)&header, sizeof(wavHeader));
}
