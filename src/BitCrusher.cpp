#include "BitCrusher.h"

BitCrusher::BitCrusher(){


}

void BitCrusher::applyEffect(int16_t *inBuffer, int16_t *outBuffer, int numBytes){

	uint32_t beginTime = micros();

	for(int i = 0; i < numBytes/2; ++i){

		outBuffer[i] = signalProcessing(inBuffer[i]);
	}

	printf("It takes %ld time to process %d samples.\n",micros()-beginTime, numBytes/2);

}

int16_t BitCrusher::signalProcessing(int16_t sample){

	sample /= scaleFactor;
	sample *= scaleFactor;

	return sample;
}

void BitCrusher::setIntensity(uint8_t intensity){

	/// max intensity = 236

	scaleFactor = (intensity / 237.0f) * pow(2, 14);
	scaleFactor = max((uint16_t) 1, scaleFactor);

}