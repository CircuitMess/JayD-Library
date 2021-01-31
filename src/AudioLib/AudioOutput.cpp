#include "AudioOutput.h"
#include "AudioGenerator.h"
#include <Loop/LoopManager.h>

AudioOutput::AudioOutput() : 
		inBuffer(nullptr),
		gain(1.0),
		generator(nullptr),
		running(false){

	inBuffer = (int16_t*)calloc(bufferSize, sizeof(int16_t));
}

AudioOutput::~AudioOutput(){
	if(inBuffer != nullptr){
		free(inBuffer);
	}
}

void AudioOutput::setSource(AudioGenerator* generator){
	this->generator = generator;
}

void AudioOutput::loop(uint _time){
	if(generator != nullptr){
		int receivedSamples = generator->generate(inBuffer);
		if(receivedSamples == 0){
			stop();
			return;
		}
		for(uint32_t i = 0; i < receivedSamples/sizeof(int16_t); i++){
			*(inBuffer + i) = static_cast<int16_t>((*(inBuffer + i)) * gain);
		}
	}
	output();
}

void AudioOutput::stop(){
	running = false;
	LoopManager::removeListener(this);
}

void AudioOutput::start(){
	running = true;
	LoopManager::addListener(this);
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