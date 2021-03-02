#ifndef JAYD_EFFECTPROCESSOR_H
#define JAYD_EFFECTPROCESSOR_H

#include <Arduino.h>
#include "Generator.h"
#include "Effect.h"
#include <vector>

class EffectProcessor : public Generator
{
public:
	EffectProcessor(Generator* generator);
	~EffectProcessor();
	int generate(int16_t* outBuffer) override;
	void addEffect(Effect* effect);
	void removeEffect(int index);
	Effect* getEffect(int index);

private:
	std::vector<Effect*> effectList;
	int16_t *effectBuffer;
	Generator* inputGenerator;
};
#endif