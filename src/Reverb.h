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

	int16_t signalProcessing(uint32_t index, uint32_t delay0, uint32_t delay1, uint32_t delay2, uint32_t delay3);

	float* samplesBuffer = nullptr;
	uint8_t sampleBufferCnt;

	bool startReverb;
	float decayFactor;
	float mixPercent;

	float maxAmp;
	float threshold;
};


#endif //JAYD_LIBRARY_REVERB_H
