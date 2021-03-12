#include "EffectProcessor.h"
#include "../AudioSetup.hpp"
EffectProcessor::EffectProcessor(Generator* generator) : inputGenerator(generator)
{
	effectBuffer = (int16_t*)calloc(BUFFER_SAMPLES, BYTES_PER_SAMPLE);
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
	int receivedBytes = inputGenerator->generate(effectBuffer);
	if(effectList.empty()){
		memcpy(outBuffer, effectBuffer, receivedBytes);
		return receivedBytes;
	}
	for(uint8_t i = 0; i < effectList.size(); i++){
		Effect* effect = effectList[i];
		if(effect != nullptr){
			effect->applyEffect((i%2 == 0) ? effectBuffer, outBuffer : outBuffer, effectBuffer, receivedBytes);
		}
	}
	if(effectList.size() % 2 == 0){
		memcpy(outBuffer, effectBuffer, receivedBytes);
	}
	return receivedBytes;
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