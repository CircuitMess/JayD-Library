
#include "LowPassFilter.h"

LowPassFilter::LowPassFilter(){


}

void LowPassFilter::applyEffect(int16_t *inBuffer, int16_t *outBuffer, int numSamples){

	for(int i = 0; i < numSamples/2; ++i){

		*(outBuffer + i) = signalProcessing(*(inBuffer + i));

	}

}

void LowPassFilter::idealFilterResponse(){

	for(int n = -N/2; n <= N/2; ++n){

		if(n==0){

			idealFilterCoeffs[n+N/2] = cutOffFrequency/PI;
		}
		else{

			idealFilterCoeffs[n+N/2] = sin((float)n*cutOffFrequency)/((float)n*PI);
		}
	}
}

void LowPassFilter::windowResponse(){

	for(int n = -(L-1)/2; n <= (L-1)/2; ++n){

		windowCoeffs[n + (L-1)/2] = 0.42 + 0.5*cos((2*PI*n)/(L-1)) + 0.08*cos((4*PI*n)/(L-1));	// Blackman

		//float alpha = 0.5;
		//windowCoeffs[n + (L-1)/2] = 0.5 + (1-alpha)*cos((2*PI*n)/(L-1));	// Hann
	}

}

void LowPassFilter::generateFilterCoeffs(){

	idealFilterResponse();
	windowResponse();

	for(int n = 0; n < L; ++n){
		coeffs[n] = windowCoeffs[n] * idealFilterCoeffs[n];
	}
}

int LowPassFilter::circularIndex(int index, int increment, uint8_t irl){

	index += increment;

	if (index < 0){
		index += irl;
	}
	else if(index >= irl){
		index -= irl;
	}

	return index;
}

int16_t LowPassFilter::signalProcessing(int16_t sample){

	int i;
	int32_t acc = 0;

	fDelay[j] = sample;

	j = circularIndex(j, 1, L);

	for(i = 0; i<L; ++i){

		acc += coeffs[i]*(float)fDelay[j]*gain;
		j = circularIndex(j, 1, L);
	}


	uint16_t maxAmp = pow(2,15)-1;
	int32_t treshold = maxAmp * 0.9;
	uint16_t window = maxAmp - treshold;

	if(sample < 0){

		treshold = -treshold;
	}
	//printf("%d %d\n", sample, acc);

	if(abs(acc) >= abs(treshold)){

		int16_t over = acc - treshold;
		float overC = (float)over/(float)window;

		acc = treshold + (float)over/(1.0/cos(overC*PI/2.0));

		return acc;
	}
	else
		return acc;
}

void LowPassFilter::setIntensity(uint8_t intensity){

	//intensity = 1.0f / (float)intensity + 1;

	intensity = (float)intensity/1.08f + 20;

	cutOffFrequency = ((float)intensity/255.0f)*(float)PI;

	if(intensity < 1){
		intensity = 1;
	}

	gain = 4.0f/pow(intensity,2) + 1.0f;
	//gain = (1.0f/((float)intensity+1.0f)) * 1.0f + 1;

	generateFilterCoeffs();
}
