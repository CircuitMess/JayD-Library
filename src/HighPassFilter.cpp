//
// Created by Domagoj on 18/02/2021.
//

#include "HighPassFilter.h"

HighPassFilter::HighPassFilter(){


}

void HighPassFilter::applyEffect(int16_t *inBuffer, int16_t *outBuffer, int numSamples){

	//uint32_t beginTime = micros();

	for(int i = 0; i < numSamples/2; ++i){

		outBuffer[i] = signalProcessing(inBuffer[i]);
	}

	//printf("It takes %ld time to process %d samples.\n",micros()-beginTime, numSamples/2);
}

void HighPassFilter::idealFilterResponse(){

	for(int n = -N/2; n <= N/2; ++n){

		if(n==0){

			idealFilterCoeffs[n+N/2] = 1 - (cutOffFrequency/PI);

		}
		else{

			idealFilterCoeffs[n+N/2] = -sin((float)n*cutOffFrequency)/((float)n*PI);
		}
	}
}

void HighPassFilter::windowResponse(){

	for(int n = -(L-1)/2; n <= (L-1)/2; ++n){

		//windowCoeffs[n + (L-1)/2] = 0.42 + 0.5*cos((2*PI*n)/(L-1)) + 0.08*cos((4*PI*n)/(L-1));	// Blackman

		float alpha = 0.5;
		windowCoeffs[n + (L-1)/2] = 0.5 + (1-alpha)*cos((2*PI*n)/(L-1));	// Hann
	}
}

void HighPassFilter::generateFilterCoeffs(){

	idealFilterResponse();
	windowResponse();

	for(int n = 0; n < L; ++n){
		coeffs[n] = windowCoeffs[n] * idealFilterCoeffs[n];
	}

}

long HighPassFilter::circularIndex(long index, long increment, uint8_t irl){

	index += increment;

	if (index < 0){
		index += irl;
	}
	else if(index >= irl){
		index -= irl;
	}

	return index;
}

int16_t HighPassFilter::signalProcessing(int16_t sample){

	float acc = 0;

	fDelay[j] = sample;

	j = circularIndex(j, 1, L);

	for(int i = 0; i<L; ++i){

		acc += coeffs[i]*fDelay[j]*gain;
		j = circularIndex(j, 1, L);
	}

	float maxAmp = pow(2,15)-1;
	float threshold = maxAmp * 0.9;
	float window = maxAmp - threshold;

	if(sample < 0){

		threshold = -threshold;
	}

	if(abs(acc) >= abs(threshold)){

		float over = acc - threshold;
		float overC = over/window;

		acc = threshold + over * cos(overC*M_PI_2);

		return (int16_t)acc;
	}
	else
		return (int16_t)acc;
}

void HighPassFilter::setIntensity(uint8_t intensity){

	intensity = intensity/8;

	cutOffFrequency = ((float)intensity/236.0f)*(float)PI;

	if(intensity < 1){
		intensity = 1;
	}

	gain = (intensity/236.0f)*1.0f + 1.0f;

	generateFilterCoeffs();

}
