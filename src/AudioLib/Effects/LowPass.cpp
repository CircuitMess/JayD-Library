#include "LowPass.h"

LowPass::LowPass(){
	setIntensity(0);
}

void LowPass::applyEffect(int16_t *inBuffer, int16_t *outBuffer, size_t numSamples){
	for(int i = 0; i < numSamples; ++i){
		outBuffer[i] = signalProcessing(inBuffer[i]);
	}
}

int16_t LowPass::signalProcessing(int16_t sample){

	filter = ((float)sample * fAmpI) + (filter * fAmp);
	filter2 = (filter * fAmpI) + (filter2 * fAmp);
	sample = filter2*gain;

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

void LowPass::setIntensity(uint8_t intensity){
	intensity *= 0.9f;

	gain = exp(intensity/255.0f) / 2.0f;

	val = (float)intensity/255.0f;
	fAmp = val;
	fAmpI = 1 - val;
}
