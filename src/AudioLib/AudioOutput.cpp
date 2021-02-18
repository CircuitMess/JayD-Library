#include "AudioOutput.h"
#include "AudioGenerator.h"
#include <Loop/LoopManager.h>
#include "DefaultAudioSettings.hpp"

AudioOutput::AudioOutput() : 
		inBuffer(nullptr),
		gain(1.0),
		generator(nullptr),
		running(false){

	inBuffer = (int16_t*)calloc(DEFAULT_BUFFSIZE, DEFAULT_BYTESPERSAMPLE);
}

AudioOutput::~AudioOutput(){
	LoopManager::removeListener(this);
	if(inBuffer != nullptr){
		free(inBuffer);
	}
}

void AudioOutput::setSource(AudioGenerator* generator){
	this->generator = generator;
}

void AudioOutput::loop(uint _time){
	size_t receivedBytes = 0;
	if(generator != nullptr){
		receivedBytes = generator->generate(inBuffer);
		if(receivedBytes == 0 && running){
			stop();
			running = false;
			return;
		} else if(receivedBytes != 0 && !running){
			start();
			running = true;
		}
		for(uint32_t i = 0; i < receivedBytes/DEFAULT_BYTESPERSAMPLE; i++){
			*(inBuffer + i) = static_cast<int16_t>((*(inBuffer + i)) * gain);
		}
	}
	if(running){
		output(receivedBytes);
	}
}

bool AudioOutput::isRunning(){
	return running;
}

void AudioOutput::setGain(float _gain){
	gain = _gain;
}

float AudioOutput::getGain(){
	return gain;
}