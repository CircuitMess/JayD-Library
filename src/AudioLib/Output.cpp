#include "Output.h"
#include "Generator.h"
#include <Loop/LoopManager.h>
#include "../AudioSetup.hpp"

Output::Output(bool timed) :
		generator(nullptr),
		timed(timed){

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

#define microsPerBuffer ((float) BUFFER_SAMPLES * (1000000.0f / (float) SAMPLE_RATE))
void Output::loop(uint _time){
	if(generator == nullptr) return;
	if(!running) return;

	if(timed){
		// TODO: check timings
		uint32_t m = micros();
		if(m - lastSample <= 0.995f * microsPerBuffer) return;
		output(receivedSamples);
		lastSample += microsPerBuffer;
	}else{
		output(receivedSamples);
	}

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
		inBuffer[i] = (float) inBuffer[i] * gain;
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