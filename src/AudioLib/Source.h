#ifndef JAYD_SOURCE_H
#define JAYD_SOURCE_H

#include "Generator.h"

class Source : public Generator
{
public:
	virtual int getBitsPerSample() = 0;
	virtual int getSampleRate() = 0;
	virtual int getChannels() = 0;
};

#endif