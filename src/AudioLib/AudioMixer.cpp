#include "AudioMixer.h"

AudioMixer::AudioMixer()
{
}

AudioMixer::~AudioMixer()
{
	for(int16_t* buffer : bufferList){
		if(buffer != nullptr){
			free(buffer);
		}
	}
	for(AudioGenerator* generator : generatorList){
		if(generator != nullptr){
			delete generator;
		}
	}
}

void AudioMixer::addGenerator(AudioGenerator* generator){
	generatorList.push_back(generator);
	bufferList.push_back((int16_t*)calloc(800, sizeof(int16_t)));
}

int AudioMixer::generate(int16_t *outBuffer){
	memset(outBuffer, 0, 800*sizeof(int16_t));
	int receivedSamples[generatorList.size()] = {0};

	for(uint8_t i = 0; i < generatorList.size(); i++){
		AudioGenerator* generator = generatorList[i];
		int16_t* buffer = bufferList[i];
		if(generator != nullptr && buffer != nullptr){
			receivedSamples[i] = generator->generate(buffer);
		}
	}

	for(uint16_t i = 0; i < 800; i++){
		for(int16_t* buffer : bufferList){
			if(buffer != nullptr){
				outBuffer[i]+=buffer[i];
			}
		}
	}
	return *std::max_element(receivedSamples, receivedSamples + generatorList.size());
}