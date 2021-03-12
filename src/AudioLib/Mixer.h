#ifndef JAYD_MIXER_H
#define JAYD_MIXER_H

#include <Arduino.h>
#include "Generator.h"
#include "Source.h"

class Mixer : public Generator
{
public:
	Mixer();
	~Mixer();
	size_t generate(int16_t* outBuffer) override;
	int available() override;

	void addSource(Source* source);
	uint8_t getMixRatio();
	void setMixRatio(uint8_t ratio);
	
private:
	std::vector<Source*> sourceList;
	std::vector<int16_t*> bufferList;
	uint8_t mixRatio = 122; //half-half by default, 0 = only first track, 255 = only second track
};



#endif