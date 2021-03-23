#include "HighPass.h"

HighPass::HighPass(){

}

void HighPass::applyEffect(int16_t *inBuffer, int16_t *outBuffer, int numSamples){

	for(int i = 0; i < numSamples/2; ++i){

		outBuffer[i] = signalProcessing(inBuffer[i]);
	}
}

int16_t HighPass::signalProcessing(int16_t sample){

	filter = filter2;
	filter2 = sample;
	sample = ((float)sample * fAmpI) + ((filter2 - filter) * fAmp)*gain;

	float maxAmp = pow(2,15)-1;
	float threshold = maxAmp * 0.9;
	float window = maxAmp - threshold;

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

void HighPass::setIntensity(uint8_t intensity){

	gain = exp(intensity/255.0f) / 2.0f;

	val = (float)intensity/255.0f;
	fAmp = val;
	fAmpI = 1 - val;

}
