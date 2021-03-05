#include "Reverb.h"

Reverb::Reverb(){

	echo = static_cast<int16_t *>(calloc(ECHO_LEN, sizeof(int16_t)));
}


int16_t Reverb::signalProcessing(int16_t sample){

	int16_t echoSample1 = (float)echo[(ECHO_LEN + echoCount) % (ECHO_LEN)] * echoAmount;

	echo[echoCount++] = (float)sample;
	sample+=echoSample1;
	if (echoCount > ECHO_LEN){
		echoCount = 0;
	}

	return sample;
}

void Reverb::applyEffect(int16_t *inBuffer, int16_t *outBuffer, int numBytes){

	for(int i = 0; i < numBytes/2; ++i){

		outBuffer[i] = signalProcessing(inBuffer[i]);
	}

}

void Reverb::setIntensity(uint8_t intensity){

	echoAmount = (float)intensity/255.0f;
}

Reverb::~Reverb() noexcept{

	free(echo);
}
