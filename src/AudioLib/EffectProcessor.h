#ifndef JAYD_EFFECTPROCESSOR_H
#define JAYD_EFFECTPROCESSOR_H

#include <Arduino.h>
#include "AudioGenerator.h"
#include "AudioEffect.h"

class EffectProcessor : public AudioGenerator
{
public:
	EffectProcessor(AudioGenerator* generator);
	~EffectProcessor();
	int generate(int16_t* outBuffer) override;

private:
	std::vector<AudioEffect*> effectList;
	int16_t *effectBuffer;
	AudioGenerator* inputGenerator;
};
#endif