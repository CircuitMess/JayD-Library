#include "Output.h"
#include "Generator.h"
#include <Loop/LoopManager.h>
#include "../AudioSetup.hpp"

Output::Output() : 
		inBuffer(nullptr),
		gain(1.0),
		generator(nullptr),
		running(false){

	inBuffer = (int16_t*)calloc(BUFFER_SIZE, sizeof(byte));
}

Output::~Output(){
	LoopManager::removeListener(this);
	if(inBuffer != nullptr){
		free(inBuffer);
	}
}

void Output::setSource(Generator* generator){
	this->generator = generator;
}

void Output::loop(uint _time){
	if(generator == nullptr) return;

	size_t receivedSamples = 0;
	receivedSamples = generator->generate(inBuffer);
	if(receivedSamples == 0 && running){
		stop();
		running = false;
		return;
	} else if(receivedSamples != 0 && !running){
		start();
		running = true;
	}
	for(uint32_t i = 0; i < receivedSamples*NUM_CHANNELS; i++){
		*(inBuffer + i) = static_cast<int16_t>((*(inBuffer + i)) * gain);
	}
	
	if(running){
		output(receivedSamples);
	}
}

bool Output::isRunning(){
	return running;
}

void Output::setGain(float _gain){
	gain = _gain;
}

float Output::getGain(){
	return gain;
}