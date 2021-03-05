//
// Created by Domagoj on 18/02/2021.
//

#ifndef JAYD_LIBRARY_LOWPASSFILTER_H
#define JAYD_LIBRARY_LOWPASSFILTER_H

#include "JayD.hpp"

#define N 20		// filter order
#define L 21		// length of the impulse response L = (N+1)


class LowPassFilter :  public AudioEffect{

public:

	LowPassFilter();

	void applyEffect(int16_t *inBuffer, int16_t *outBuffer, int numSamples) override;

	void setIntensity(uint8_t intensity) override;

private:

	void idealFilterResponse();
	void windowResponse();

	void generateFilterCoeffs();

	int circularIndex(int index, int increment, uint8_t irl);

	int16_t signalProcessing(int16_t sample);

	float cutOffFrequency = 0;
	float gain = 0;

	float coeffs[L] = {0};
	float windowCoeffs[L] = {0};
	float idealFilterCoeffs[L] = {0};

	float fDelay[L] = {0};

	int j = 0;

};


#endif //JAYD_LIBRARY_LOWPASSFILTER_H
