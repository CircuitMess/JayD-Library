#ifndef JAYD_LIBRARY_REVERB_H
#define JAYD_LIBRARY_REVERB_H

#include "JayD.hpp"

#define ECHO_LEN 25000

class Reverb : public AudioEffect{

public:

	Reverb();

	~Reverb() override;

	void applyEffect(int16_t *inBuffer, int16_t *outBuffer, int numBytes) override;

	void setIntensity(uint8_t intensity);

private:

	int16_t signalProcessing(int16_t sample);

	int16_t* echo = nullptr;

	uint16_t echoCount = 0;
	float echoAmount = 0;

};


#endif //JAYD_LIBRARY_REVERB_H
