#include "Reverb.h"

Reverb::Reverb(){
	echo = static_cast<int16_t *>(calloc(length, sizeof(int16_t)));
}


int16_t Reverb::signalProcessing(int16_t sample){

	int16_t echoSample1 = (float)echo[(length + echoCount) % (length)] * echoAmount;

	echo[echoCount++] = (float)sample;
	sample+=echoSample1;
	if (echoCount > length){
		echoCount = 0;
	}

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

Reverb::~Reverb() noexcept{
	free(echo);
}
