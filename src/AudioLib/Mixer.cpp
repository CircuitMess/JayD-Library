#include "Mixer.h"
#include "../AudioSetup.hpp"

//clipping wave to avoid overflows
int16_t clip(int32_t input){ 

    if (input > 0x7FFF) return 0x7FFFF;
    if (input < - 0x7FFF) return -0x7FFF;
    return input;
}


Mixer::Mixer()
{
}

Mixer::~Mixer()
{
	for(int16_t* buffer : bufferList){
		if(buffer != nullptr){
			free(buffer);
		}
	}
	for(auto generator : sourceList){
		if(generator != nullptr){
			delete generator;
		}
	}
}

size_t Mixer::generate(int16_t *outBuffer){
	memset(outBuffer, 0, BUFFER_SIZE);
	int receivedSamples[sourceList.size()] = {0};

	for(uint8_t i = 0; i < sourceList.size(); i++){
		Generator* generator = sourceList[i];
		int16_t* buffer = bufferList[i];
		if(generator != nullptr && buffer != nullptr){
			receivedSamples[i] = generator->generate(buffer);
		}
	}

	for(uint16_t i = 0; i < BUFFER_SAMPLES*NUM_CHANNELS; i++){
		int32_t wave = 0;
		for(uint8_t j = 0; j < sourceList.size(); j++){
			if(bufferList[j] == nullptr || receivedSamples[j] < i/NUM_CHANNELS ) break;

			if(sourceList.size() == 2){
				wave += bufferList[j][i] * (float)((j == 1 ? (float)(mixRatio) : (float)(255.0 - mixRatio))/255.0); //use the mixer if only 2 tracks found
			}else{
				wave += bufferList[j][i] * (float)(1.0/(float)(sourceList.size())); // evenly distribute all tracks
			}
		}
		outBuffer[i] = clip(wave);
	}
	size_t longestBuffer = *std::max_element(receivedSamples, receivedSamples + sourceList.size());
	return longestBuffer;
}

int Mixer::available(){
	int available = 0;
	for(uint8_t i = 0; i < sourceList.size(); i++){
		available = max(available, sourceList[i]->available());
	}
	return available;
}

void Mixer::addSource(Generator* generator){
	sourceList.push_back(generator);
	bufferList.push_back((int16_t*)calloc(BUFFER_SIZE, sizeof(byte)));
}

void Mixer::setMixRatio(uint8_t ratio){
	mixRatio = ratio;
}

uint8_t Mixer::getMixRatio(){
	return uint8_t(mixRatio);
}