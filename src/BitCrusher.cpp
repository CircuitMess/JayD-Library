#include "BitCrusher.h"

BitCrusher::BitCrusher(){


}

void BitCrusher::applyEffect(int16_t *inBuffer, int16_t *outBuffer, int numBytes){

	for(int i = 0; i < numBytes/2; ++i){

		outBuffer[i] = signalProcessing(inBuffer[i]);
	}
}

int16_t BitCrusher::signalProcessing(int16_t sample){

	sample /= scaleFactor;
	sample *= scaleFactor;

	return sample;
}

void BitCrusher::setIntensity(uint8_t intensity){

	/// max intensity = 236

	scaleFactor = (intensity / 236.0f) * pow(2, 14);
	scaleFactor = max((uint16_t) 1, scaleFactor);

}