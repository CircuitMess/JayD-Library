#include "SourceWAV.h"
#include "../AudioSetup.hpp"

//-------------------------------------------
// Helper structs and funcs
struct wavHeader{
	char RIFF[4];
	uint32_t chunkSize;
	char WAVE[4];
	char fmt[4];
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

inline void endian_swap(unsigned int& x){
	x = (x>>24) | ((x<<8) & 0x00FF0000) | ((x>>8) & 0x0000FF00) | (x<<24);
}
inline void endian_swap(unsigned short& x){
	x = (x>>8) | (x<<8);
}
// ----------------------------------------------


SourceWAV::SourceWAV() : file(nullptr), channels(0), sampleRate(0), bitsPerSample(0){
}

SourceWAV::SourceWAV(fs::File *_file) : SourceWAV(){
	file = _file;
}

SourceWAV::~SourceWAV(){
}

void SourceWAV::readHeader(){
	char *buffer = (char*)malloc(sizeof(wavHeader));
	file->readBytes(buffer, sizeof(wavHeader));
	
	wavHeader *header = (wavHeader*)buffer;
	if(memcmp(header->RIFF, "RIFF", 4) != 0){
		Serial.println("Error, couldn't find RIFF ID");
		free(buffer);
		return;
	}
	if(memcmp(header->WAVE, "WAVE", 4) != 0){
		Serial.println("Error, couldn't find WAVE ID");
		free(buffer);
		return;
	}
	if(memcmp(header->fmt, "fmt ", 4) != 0){
		Serial.println("Error, couldn't find fmt ID");
		free(buffer);
		return;
	}
	if(memcmp(header->data, "data", 4) != 0){
		Serial.println("Error, couldn't find data ID");
		free(buffer);
		return;
	}

	endian_swap(header->numChannels);
	channels = header->numChannels;
	endian_swap(header->sampleRate);
	sampleRate = header->sampleRate;
	endian_swap(header->bitsPerSample);
	bitsPerSample = header->bitsPerSample;
	free(buffer);
}

size_t SourceWAV::generate(int16_t *outBuffer){
	if(file == nullptr){
		Serial.println("file nullptr");
		return 0;
	}
	if(!file){
		Serial.println("file false");
		return 0;
	}

	if(sampleRate == 0){
		readHeader();
	}

	return (file->read((uint8_t*)outBuffer, BUFFER_SIZE) / (BYTES_PER_SAMPLE*NUM_CHANNELS));
}

int SourceWAV::getBitsPerSample(){
	return bitsPerSample;
}

int SourceWAV::getSampleRate(){
	return sampleRate;
}

int SourceWAV::getChannels(){
	return channels;
}

void SourceWAV::open(fs::File *_file){
	if(file != nullptr) delete file;
	
	file = _file;
	channels, sampleRate, bitsPerSample = 0;
}

int SourceWAV::available(){
	return (file->available()/(NUM_CHANNELS*BYTES_PER_SAMPLE));
}