#ifndef JAYD_AUDIOMIXER_H
#define JAYD_AUDIOMIXER_H

#include <Arduino.h>
#include "AudioGenerator.h"
#include "AudioSource.h"

class AudioMixer : public AudioGenerator
{
public:
	AudioMixer();
	~AudioMixer();
	int generate(int16_t* outBuffer) override;
	int available() override;

	void addSource(AudioSource* source);
	uint8_t getMixRatio();
	void setMixRatio(uint8_t ratio);
	
private:
	std::vector<AudioSource*> sourceList;
	std::vector<int16_t*> bufferList;
	uint8_t mixRatio; //half-half by default, 0 = only first track, 255 = only second track
};



#endif