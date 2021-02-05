#include "AudioGeneratorMP3.h"

AudioGeneratorMP3::AudioGeneratorMP3() : file(nullptr), channels(0), sampleRate(0), bitsPerSample(0){
}

AudioGeneratorMP3::AudioGeneratorMP3(fs::File *_file) : AudioGeneratorMP3(){
	file = _file;
}

AudioGeneratorMP3::~AudioGeneratorMP3(){
}

int AudioGeneratorMP3::generate(int16_t *outBuffer){

	Serial.println("generate start");
	delay(6);
	if(file == nullptr){
		Serial.println("file nullptr");
		return 0;
	}
	if(!file){
		Serial.println("file false");
		return 0;
	}

	return 0;
}

int AudioGeneratorMP3::getBitsPerSample(){
	return bitsPerSample;
}

int AudioGeneratorMP3::getSampleRate(){
	return sampleRate;
}

int AudioGeneratorMP3::getChannels(){
	return channels;
}

void AudioGeneratorMP3::open(fs::File *_file){
	if(file != nullptr) delete file;
	
	file = _file;
	channels, sampleRate, bitsPerSample = 0;
}

int AudioGeneratorMP3::available(){
	return file->available();
}