#ifndef JAYD_AUDIOMIXER_H
#define JAYD_AUDIOMIXER_H

#include <Arduino.h>
#include "AudioGenerator.h"

class AudioMixer : public AudioGenerator
{
public:
	AudioMixer();
	~AudioMixer();
	void addGenerator(AudioGenerator* generator);
	int generate(int16_t* outBuffer) override;
	uint8_t getMixRatio();
	void setMixRatio(uint8_t ratio);
private:
	std::vector<AudioGenerator*> generatorList;
	std::vector<int16_t*> bufferList;
	uint8_t mixRatio;
};



#endif