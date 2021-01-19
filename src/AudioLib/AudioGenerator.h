#ifndef JAYD_AUDIOGENERATOR_H
#define JAYD_AUDIOGENERATOR_H
#include <Arduino.h>

class AudioGenerator
{
public:
	virtual ~AudioGenerator() = default;
	virtual int generate(int16_t *outBuffer) = 0;
	virtual int available() = 0;
};

#endif