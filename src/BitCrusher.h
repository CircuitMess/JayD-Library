#ifndef JAYD_LIBRARY_BITCRUSHER_H
#define JAYD_LIBRARY_BITCRUSHER_H

#include "JayD.hpp"

class BitCrusher : public AudioEffect {

public:

	BitCrusher();

	void applyEffect(int16_t *inBuffer, int16_t *outBuffer, int numBytes) override;
	void setIntensity(uint8_t intensity) override;

private:

	int16_t signalProcessing(int16_t sample);

	uint16_t scaleFactor;
};


#endif //JAYD_LIBRARY_BITCRUSHER_H