#ifndef JAYD_AUDIOSOURCE_H
#define JAYD_AUDIOSOURCE_H

#include "AudioGenerator.h"

class AudioSource : public AudioGenerator
{
public:
	virtual int getBitsPerSample() = 0;
	virtual int getSampleRate() = 0;
	virtual int getChannels() = 0;
};

#endif