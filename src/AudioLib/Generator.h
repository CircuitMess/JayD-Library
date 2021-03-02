#ifndef JAYD_GENERATOR_H
#define JAYD_GENERATOR_H
#include <Arduino.h>

class Generator
{
public:
	virtual ~Generator() = default;
	virtual int generate(int16_t *outBuffer) = 0;
	virtual int available() = 0;
};

#endif