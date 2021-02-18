#include "EffectProcessor.h"
#include "DefaultAudioSettings.hpp"
EffectProcessor::EffectProcessor(AudioGenerator* generator) : effectBuffer(nullptr), inputGenerator(generator)
{
	effectBuffer = (int16_t*)calloc(DEFAULT_BUFFSIZE, DEFAULT_BYTESPERSAMPLE);
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
	int receivedBytes = inputGenerator->generate(effectBuffer);
	if(effectList.empty()){
		memcpy(outBuffer, effectBuffer, receivedBytes);
		return receivedBytes;
	}
	for(uint8_t i = 0; i < effectList.size(); i++){
		AudioEffect* effect = effectList[i];
		if(effect != nullptr){
			effect->applyEffect((i%2 == 0) ? effectBuffer, outBuffer : outBuffer, effectBuffer, receivedBytes);
		}
	}
	if(effectList.size() % 2 == 0){
		memcpy(outBuffer, effectBuffer, receivedBytes);
	}
	return receivedBytes;
}

void EffectProcessor::addEffect(AudioEffect* effect){
	effectList.push_back(effect);
}

void EffectProcessor::removeEffect(int index){
	effectList.erase(effectList.begin() + index);
}

AudioEffect* EffectProcessor::getEffect(int index){
	return effectList[index];
}