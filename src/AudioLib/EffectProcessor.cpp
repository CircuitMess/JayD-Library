#include "EffectProcessor.h"

EffectProcessor::EffectProcessor(AudioGenerator* generator) : effectBuffer(nullptr), inputGenerator(generator)
{
	effectBuffer = (int16_t*)calloc(800, sizeof(int16_t));
}

EffectProcessor::~EffectProcessor()
{
	if(effectBuffer != nullptr){
		free(effectBuffer);
	}
	for(AudioEffect* effect : effectList){
		if(effect != nullptr){
			delete effect;
		}
	}
	if(inputGenerator != nullptr){
		delete inputGenerator;
	}
}

int EffectProcessor::generate(int16_t* outBuffer){
	int receivedSamples = inputGenerator->generate(effectBuffer);
	if(effectList.empty()){
		memcpy(outBuffer, effectBuffer, receivedSamples*sizeof(int16_t));
		return receivedSamples;
	}
	for(uint8_t i = 0; i < effectList.size(); i++){
		AudioEffect* effect = effectList[i];
		if(effect != nullptr){
			effect->applyEffect((i%2 == 0) ? effectBuffer, outBuffer : outBuffer, effectBuffer, receivedSamples);
		}
	}
	if(effectList.size() % 2 == 0){
		memcpy(outBuffer, effectBuffer, receivedSamples*sizeof(int16_t));
	}
	return receivedSamples;
}