#ifndef JAYD_LIBRARY_REVERB_H
#define JAYD_LIBRARY_REVERB_H

#include <Arduino.h>
#include "../Effect.h"

class Reverb : public Effect{

public:

	Reverb();

	~Reverb() override;

	void applyEffect(int16_t *inBuffer, int16_t *outBuffer, size_t numSamples) override;

	void setIntensity(uint8_t intensity);

private:
	static const uint32_t length = 25000;

	int16_t signalProcessing(int16_t sample);

	int16_t* echo = nullptr;

	uint16_t echoCount = 0;
	float echoAmount = 0;

};


#endif //JAYD_LIBRARY_REVERB_H
