#include "Reverb.h"

Reverb::Reverb(){
	if(psramFound()){
		echo = static_cast<int16_t *>(ps_calloc(length, sizeof(int16_t)));
	}else{
		echo = static_cast<int16_t *>(calloc(length, sizeof(int16_t)));
	}
}


int16_t Reverb::signalProcessing(int16_t sample){

	int16_t echoSample1 = (float)echo[(length + echoCount) % (length)] * echoAmount;

	echo[echoCount++] = (float)sample;
	if (echoCount >= length){
		echoCount = 0;
	}

	sample+=echoSample1;

	return sample;
}

void Reverb::applyEffect(int16_t *inBuffer, int16_t *outBuffer, size_t numSamples){
	for(int i = 0; i < numSamples; ++i){
		outBuffer[i] = signalProcessing(inBuffer[i]);
	}
}

void Reverb::setIntensity(uint8_t intensity){
	echoAmount = (float)intensity/255.0f;
}

Reverb::~Reverb(){
	free(echo);
}
