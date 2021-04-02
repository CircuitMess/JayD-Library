#include "Reverb.h"

Reverb::Reverb(){
	if(psramFound()){
		echo = static_cast<int16_t *>(ps_calloc(length, sizeof(int16_t)));
	}else{
		echo = static_cast<int16_t *>(calloc(length, sizeof(int16_t)));
	}
}


int16_t Reverb::signalProcessing(int16_t sample){

	int16_t echoSample1 = (float) echo[(length + echoCount) % (length)] * echoAmount;

	echo[echoCount++] = (float) sample;
	if(echoCount >= length){
		echoCount = 0;
	}

	float maxAmp = pow(2,15)-1;
	float threshold = maxAmp * 0.9;
	float window = maxAmp - threshold;

	if(abs(sample+echoSample1)> 0x7FFF){

		sample = (float)sample*(0.5f*((1 - echoAmount) + 1)) + (float)echoSample1*(echoAmount*0.5f);
	}
	else{
		sample+=echoSample1;
	}

	if(sample < 0){

		threshold = -threshold;
	}

	if(abs(sample) >= abs(threshold)){

		float over = (float)sample - threshold;
		float overC = over/window;

		sample = threshold + over * cos(overC*M_PI_2);

		return (int16_t)sample;
	}
	else
		return (int16_t)sample;
}


void Reverb::applyEffect(int16_t *inBuffer, int16_t *outBuffer, size_t numSamples){
	for(int i = 0; i < numSamples; ++i){
		outBuffer[i] = signalProcessing(inBuffer[i]);
	}
}

void Reverb::setIntensity(uint8_t intensity){
	echoAmount = (float) intensity / 255.0f;
}

Reverb::~Reverb(){
	free(echo);
}
