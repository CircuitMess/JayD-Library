#include "EffectProcessor.h"
#include "../AudioSetup.hpp"
EffectProcessor::EffectProcessor(Generator* generator) : inputGenerator(generator)
{
	effectBuffer = (int16_t*) calloc(1, BUFFER_SIZE);
}

EffectProcessor::~EffectProcessor()
{
	if(effectBuffer != nullptr){
		free(effectBuffer);
	}
	for(Effect* effect : effectList){
		if(effect != nullptr){
			delete effect;
		}
	}
	if(inputGenerator != nullptr){
		delete inputGenerator;
	}
}

size_t EffectProcessor::generate(int16_t* outBuffer){
	size_t noSamples = inputGenerator->generate(effectBuffer);
	size_t bytes = noSamples * BYTES_PER_SAMPLE * NUM_CHANNELS;
	if(effectList.empty()){
		memcpy(outBuffer, effectBuffer, bytes);
		return noSamples;
	}
	for(size_t i = 0; i < effectList.size(); i++){
		Effect* effect = effectList[i];
		if(effect == nullptr) continue;

		if(i%2 == 0){
			effect->applyEffect(effectBuffer, outBuffer, noSamples);
		}else{
			effect->applyEffect(outBuffer, effectBuffer, noSamples);
		}
	}
	if(effectList.size() % 2 == 0){
		memcpy(outBuffer, effectBuffer, bytes);
	}

	return noSamples;
}

void EffectProcessor::addEffect(Effect* effect){
	effectList.push_back(effect);
}

void EffectProcessor::removeEffect(int index){
	effectList.erase(effectList.begin() + index);
}

Effect* EffectProcessor::getEffect(int index){
	return effectList[index];
}

int EffectProcessor::available(){
	if(inputGenerator == nullptr) return 0;
	return inputGenerator->available();
}