#ifndef JAYD_LIBRARY_REVERB_H
#define JAYD_LIBRARY_REVERB_H

#include "JayD.hpp"

class Reverb : public AudioEffect{

public:

	Reverb();
	~Reverb() override;

	void applyEffect(int16_t *inBuffer, int16_t *outBuffer, int numBytes) override;
	void setIntensity(uint8_t intensity);

private:

	void combFilter(int16_t* sample, int numberOfSamples);
	void allPassFilter(int32_t* sample, int numberOfSamples, uint8_t index);

	void signalProcessing(int16_t* samples, int numBytes);

	int16_t* samplesBuffer = nullptr;
	uint8_t sampleBufferCnt;
	bool startReverb;

	int32_t* combFilter1Output = nullptr;
	int32_t* combFilter2Output = nullptr;
	int32_t* combFilter3Output = nullptr;
	int32_t* combFilter4Output = nullptr;

	int32_t * allPassFilter1Output = nullptr;
	int32_t * allPassFilter2Output = nullptr;

	float delaySamples;
	float decayFactor;

	int delayAllPassSamples;
	float allPassDecay;

	uint8_t mixPercent;
};


#endif //JAYD_LIBRARY_REVERB_H
