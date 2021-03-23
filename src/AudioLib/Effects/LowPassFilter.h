#ifndef JAYD_LIBRARY_LOWPASSFILTER_H
#define JAYD_LIBRARY_LOWPASSFILTER_H

#include <Arduino.h>
#include "../Effect.h"

class LowPassFilter :  public Effect{

public:

	LowPassFilter();

	void applyEffect(int16_t *inBuffer, int16_t *outBuffer, int numSamples) override;

	void setIntensity(uint8_t intensity) override;

private:

	int16_t signalProcessing(int16_t sample);

	float gain = 0;

	int j = 0;

	float filter;
	float filter2;

	float val;
	float fAmp;
	float fAmpI;
};

#endif //JAYD_LIBRARY_LOWPASSFILTER_H
