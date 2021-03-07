#include "OutputFS.h"
#include <string>
#include "../AudioSetup.hpp"
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
		Output::Output(), path(path), filesystem(filesystem), recordingNum(0){
}

OutputFS::~OutputFS(){
	if(file != nullptr){
		file->close();
		delete file;
	}
}

void OutputFS::output(size_t numSamples){
	file->write((uint8_t*)inBuffer, numSamples*NUM_CHANNELS*BYTES_PER_SAMPLE);
	dataLength+=numSamples*NUM_CHANNELS*BYTES_PER_SAMPLE;
}

void OutputFS::start(){
	dataLength = 0;
	file = new fs::File(filesystem->open(path, "w"));
	if(!(*file)){
		Serial.println("Couldn't open file for output");
		return;
	}
	writeHeader();
}

void OutputFS::stop(){
	char name[100];
	strncpy(name, file->name(), 100);
	file->close();
	delete file;

	file = new fs::File(filesystem->open(name, "r+"));
	char *buffer = (char*)malloc(sizeof(wavHeader));
	file->seek(0);
	file->readBytes(buffer, sizeof(wavHeader));
	wavHeader *header = (wavHeader*)buffer;
	header->chunkSize = dataLength + 36;
	header->dataSize = dataLength;
	file->seek(0);
	file->write((uint8_t*)header, sizeof(wavHeader));

	file->close();
	recordingNum++;
	free(buffer);
}

void OutputFS::writeHeader(){
	wavHeader header;
	memcpy(header.RIFF, "RIFF", 4);
	header.chunkSize = 0; //corrected when stop() is called
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
	header.dataSize = 0; //corrected when stop() is called

	file->seek(0);
	file->write((uint8_t*)&header, sizeof(wavHeader));
}