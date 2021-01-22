#include "AudioGeneratorWAV.h"

//-------------------------------------------
// Helper structs and funcs
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

inline void endian_swap(unsigned int& x){
	x = (x>>24) | ((x<<8) & 0x00FF0000) | ((x>>8) & 0x0000FF00) | (x<<24);
}
inline void endian_swap(unsigned short& x){
	x = (x>>8) | (x<<8);
}
// ----------------------------------------------


AudioGeneratorWAV::AudioGeneratorWAV() : file(nullptr), channels(0), sampleRate(0), bitsPerSample(0){
}

AudioGeneratorWAV::AudioGeneratorWAV(fs::File *_file) : AudioGeneratorWAV(){
	file = _file;
}

AudioGeneratorWAV::~AudioGeneratorWAV(){
}

void AudioGeneratorWAV::readHeader(){
	char *buffer = (char*)malloc(sizeof(wavHeader));
	file->readBytes(buffer, sizeof(wavHeader));
	wavHeader *header = (wavHeader*)buffer;
	if(strcmp(header->RIFF, "RIFF") != 1){
		Serial.println("Error, couldn't find RIFF ID");
		free(buffer);
		return;
	}
	if(strcmp(header->WAVE, "WAVE") != 1){
		Serial.println("Error, couldn't find WAVE ID");
		free(buffer);
		return;
	}
	if(strcmp(header->fmt, "fmt") != 1){
		Serial.println("Error, couldn't find fmt ID");
		free(buffer);
		return;
	}
	if(strcmp(header->data, "data") != 1){
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

int AudioGeneratorWAV::generate(int16_t *outBuffer){
	if(file == nullptr) return 0;
	if(!file) return 0;

	if(sampleRate == 0){
		readHeader();
	}

	return file->read((uint8_t*)outBuffer, 800*sizeof(int16_t));
}

int AudioGeneratorWAV::getBitsPerSample(){
	return bitsPerSample;
}

int AudioGeneratorWAV::getSampleRate(){
	return sampleRate;
}

int AudioGeneratorWAV::getChannels(){
	return channels;
}

void AudioGeneratorWAV::open(fs::File *_file){
	if(file != nullptr) delete file;
	
	file = _file;
	channels, sampleRate, bitsPerSample = 0;
}