#ifndef JAYD_SOURCE_H
#define JAYD_SOURCE_H

#include "Generator.h"
#include <FS.h>

class Source : public Generator
{
public:
	virtual int getBitsPerSample() = 0;
	virtual int getSampleRate() = 0;
	virtual int getChannels() = 0;
	virtual uint16_t getDuration() = 0;
	virtual uint16_t getElapsed() = 0;
	virtual void seek(uint16_t time, fs::SeekMode mode) = 0;
};

#endif