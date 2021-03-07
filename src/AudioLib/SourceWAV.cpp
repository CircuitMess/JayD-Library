#include "SourceWAV.h"
#include "../AudioSetup.hpp"

// Number of system buffers to load at once
#define BUFFER_COUNT 2

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

// ----------------------------------------------


SourceWAV::SourceWAV() : file(nullptr), dataSize(0), readData(0){

	return;
	fileBuffer = static_cast<uint8_t*>(malloc(BUFFER_COUNT * BUFFER_SIZE));
	if(fileBuffer == nullptr){
		Serial.println("File buffer malloc error");
	}
}

SourceWAV::SourceWAV(fs::File *_file) : SourceWAV(){
	file = _file;
	readHeader();
}

SourceWAV::~SourceWAV(){
	free(fileBuffer);
}

bool SourceWAV::readHeader(){
	if(file == nullptr){
		Serial.println("file nullptr");
		return false;
	}
	if(!*file){
		Serial.println("file false");
		return false;
	}

	file->seek(0);

	char *buffer = (char*)malloc(sizeof(wavHeader));
	if(file->readBytes(buffer, sizeof(wavHeader)) != sizeof(wavHeader)){
		Serial.println("Error, couldn't read from file");
		free(buffer);
		return false;
	}
	wavHeader *header = (wavHeader*)buffer;
	if(memcmp(header->RIFF, "RIFF", 4) != 0){
		Serial.println("Error, couldn't find RIFF ID");
		free(buffer);
		return false;
	}
	if(memcmp(header->WAVE, "WAVE", 4) != 0){
		Serial.println("Error, couldn't find WAVE ID");
		free(buffer);
		return false;
	}
	if(memcmp(header->fmt, "fmt ", 4) != 0){
		Serial.println("Error, couldn't find fmt ID");
		free(buffer);
		return false;
	}
	if(memcmp(header->data, "data", 4) != 0){
		Serial.println("Error, couldn't find data ID");
		free(buffer);
		return false;
	}
	channels = header->numChannels;
	sampleRate = header->sampleRate;
	bytesPerSample = header->bitsPerSample / 8;
	dataSize = header->dataSize;
	free(buffer);
	return true;
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

	if(sampleRate == 0 || channels == 0 || bytesPerSample == 0){
		if(!readHeader()) return 0;
	}


	if(fileBuffer == nullptr){
		int readBytes = file->read((uint8_t*)outBuffer, BUFFER_SIZE);
		readData+=readBytes;
		return readBytes / (BYTES_PER_SAMPLE * NUM_CHANNELS);
	}

	if(fbPtr == 0){
		file->read(fileBuffer, BUFFER_COUNT * BUFFER_SIZE);
	}

	memcpy(outBuffer, fileBuffer + fbPtr * BUFFER_SIZE, BUFFER_SIZE);

	fbPtr++;
	if(fbPtr == BUFFER_COUNT){
		fbPtr = 0;
	}

	readData += BUFFER_SIZE;
	return BUFFER_SIZE;
}

void SourceWAV::open(fs::File *_file){
	if(file != nullptr) delete file;
	
	file = _file;
	channels = sampleRate = bytesPerSample = 0;
}

int SourceWAV::available(){
	if(sampleRate == 0 || channels == 0 || bytesPerSample == 0) return 0;
	return (file->available()/(channels*bytesPerSample));
}

uint16_t SourceWAV::getDuration(){
	if(sampleRate == 0 || channels == 0 || bytesPerSample == 0) return 0;
	return int(dataSize/(channels*bytesPerSample*sampleRate));
}

uint16_t SourceWAV::getElapsed(){
	if(sampleRate == 0 || channels == 0 || bytesPerSample == 0 || readData == 0) return 0;
	return int(readData/(channels*bytesPerSample*sampleRate));
}

void SourceWAV::seek(uint16_t time, fs::SeekMode mode){
	if(sampleRate == 0 || channels == 0 || bytesPerSample == 0 ) return;
	size_t offset = time*sampleRate*channels*bytesPerSample;
	if(offset >= file->size()) return;

	file->seek(offset, mode);
}
