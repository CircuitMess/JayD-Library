//
// Created by Domagoj on 18/02/2021.
//

#include "HighPassFilter.h"

HighPassFilter::HighPassFilter(){

}

void HighPassFilter::applyEffect(int16_t *inBuffer, int16_t *outBuffer, int numSamples){

	for(int i = 0; i < numSamples/2; ++i){

		outBuffer[i] = signalProcessing(inBuffer[i]);
	}
}

int16_t HighPassFilter::signalProcessing(int16_t sample){

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

void HighPassFilter::setIntensity(uint8_t intensity){

	gain = exp(intensity/255.0f) / 2.0f;

	val = (float)intensity/255.0f;
	fAmp = val;
	fAmpI = 1 - val;

}
