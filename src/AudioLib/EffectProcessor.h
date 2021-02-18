#ifndef JAYD_EFFECTPROCESSOR_H
#define JAYD_EFFECTPROCESSOR_H

#include <Arduino.h>
#include "AudioGenerator.h"
#include "AudioEffect.h"
#include <vector>

class EffectProcessor : public AudioGenerator
{
public:
	EffectProcessor(AudioGenerator* generator);
	~EffectProcessor();
	int generate(int16_t* outBuffer) override;
	void addEffect(AudioEffect* effect);
	void removeEffect(int index);
	AudioEffect* getEffect(int index);

private:
	std::vector<AudioEffect*> effectList;
	int16_t *effectBuffer;
	AudioGenerator* inputGenerator;
};
#endif