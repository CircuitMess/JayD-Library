#include "OutputFS.h"
#include <string>
#include "../AudioSetup.hpp"
#include "../PerfMon.h"
#include "../Services/SDScheduler.h"


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

OutputFS::OutputFS(const char* path, fs::FS* filesystem) :
		Output::Output(), path(path), filesystem(filesystem){
}

OutputFS::~OutputFS(){
	if(file != nullptr){
		file->close();
		delete file;
	}
}

void* outputBuffer = nullptr; // The output buffer size should be 6144 bits per channel excluding the LFE channel.

static void* inBuffer[] = { inBuffer };
static INT inBufferIds[] = { IN_AUDIO_DATA };
static INT inBufferSize[] = { BUFFER_SAMPLES * NUM_CHANNELS };
static INT inBufferElSize[] = { BYTES_PER_SAMPLE };

static void* outBuffer[] = { outputBuffer };
static INT outBufferIds[] = { OUT_BITSTREAM_DATA };
static INT outBufferSize[] = { sizeof(outputBuffer) };
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

#define WB_COUNT 4
uint8_t* writeBuffer = static_cast<uint8_t*>(malloc(BUFFER_SIZE * WB_COUNT));
uint8_t* wbPtr = static_cast<uint8_t*>(writeBuffer);
int wbi = 0;

void OutputFS::output(size_t numSamples){
	Profiler.start("AAC encode");
	::inBuffer[0] = this->inBuffer;
	inArgs.numInSamples = numSamples * NUM_CHANNELS;
	AACENC_OutArgs outArgs;
	int status = aacEncEncode(encoder, &inBufDesc, &outBufDesc, &inArgs, &outArgs);
	if(status != AACENC_OK){
		Serial.printf("ENC: %x\n", status);
	}
	Profiler.end();

	if(outArgs.numOutBytes != 0){
		memcpy(wbPtr, outputBuffer, outArgs.numOutBytes);
		wbPtr += outArgs.numOutBytes;
		wbi++;

		if(wbi == WB_COUNT){
			Profiler.start("AAC write");
			addWriteJob(writeBuffer, wbPtr - writeBuffer);
			Profiler.end();

			wbi = 0;
			wbPtr = static_cast<uint8_t*>(writeBuffer);
		}
	}
}

void OutputFS::start(){
	dataLength = 0;
	file = new fs::File(filesystem->open(path, "w"));
	if(!(*file)){
		Serial.println("Couldn't open file for output");
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

	size_t outSize = max(8192, NUM_CHANNELS * 768);

	Serial.printf("FRAME LENGTH: %ld\n", outSize);
	free(outputBuffer);
	outputBuffer = malloc(outSize);
	outBuffer[0] = outputBuffer;
	outBufferSize[0] = outSize;

	setupBuffers();
	file->seek(0);
}

void OutputFS::stop(){
	inArgs.numInSamples = -1;
	AACENC_OutArgs outArgs;
	int status = aacEncEncode(encoder, nullptr, &outBufDesc, &inArgs, &outArgs);
	if(status != AACENC_OK){
		Serial.printf("ENC flush: %d\n", status);
	}

	if(outArgs.numOutBytes != 0){
		Profiler.start("AAC write");
		addWriteJob(outputBuffer, outArgs.numOutBytes);
		Profiler.end();
	}

	aacEncClose(&encoder);

	file->close();
}

void OutputFS::addWriteJob(void* buffer, size_t size){
	if(writePending && writeResult == nullptr){
		while(writeResult == nullptr){
			delayMicroseconds(1);
		}

		writePending = false;
	}

	delete writeResult;
	writeResult = nullptr;

	Sched.addJob({
			 .type = SDJob::SD_WRITE,
			 .file = *file,
			 .size = size,
			 .buffer = static_cast<uint8_t*>(buffer),
			 .result = &writeResult
	 });

	writePending = true;
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

	file->seek(0);
	file->write((uint8_t*)&header, sizeof(wavHeader));
}