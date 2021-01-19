#include "AudioGeneratorWAV.h"

AudioGeneratorWAV::AudioGeneratorWAV(/* args */){
}

AudioGeneratorWAV::~AudioGeneratorWAV(){
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

void AudioGeneratorWAV::readHeader(){

}

int AudioGeneratorWAV::generate(void *outBuffer){
	
}