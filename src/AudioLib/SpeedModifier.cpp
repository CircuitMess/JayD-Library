#include "SpeedModifier.h"
#include "../AudioSetup.hpp"

SpeedModifier::SpeedModifier(Source* source) : source(source){
	dataBuffer = new DataBuffer(BUFFER_SIZE * 3, true);
}

SpeedModifier::~SpeedModifier() noexcept{
	delete dataBuffer;
}

size_t SpeedModifier::generate(int16_t *outBuffer){
	if(source == nullptr) return 0;

	if(dataBuffer->readAvailable() < (float) BUFFER_SIZE * 2){
		fillBuffer();
	}

	size_t destinationPtr = 0;
	const size_t availableSamples = dataBuffer->readAvailable() / (BYTES_PER_SAMPLE * NUM_CHANNELS);
	const int16_t* samples = reinterpret_cast<const int16_t*>(dataBuffer->readData());

	while(destinationPtr < BUFFER_SAMPLES && (sourcePosition >> 16) + 1 < availableSamples){
		const size_t sourceIndex = sourcePosition >> 16;
		const int32_t fraction = (sourcePosition & 0xffff) >> 1;
		const int32_t difference = int32_t(samples[sourceIndex + 1]) - samples[sourceIndex];
		outBuffer[destinationPtr++] = samples[sourceIndex] + difference * fraction / 32768;

		advanceRate();
		sourcePosition += currentRate;
	}

	const size_t consumedSamples = sourcePosition >> 16;
	sourcePosition &= 0xffff;
	dataBuffer->readMove(consumedSamples * BYTES_PER_SAMPLE * NUM_CHANNELS);

	return destinationPtr;
}

int SpeedModifier::available(){
	if(source == nullptr) return 0;
	return source->available() + dataBuffer->readAvailable() / (BYTES_PER_SAMPLE * NUM_CHANNELS);
}

void SpeedModifier::setModifier(uint8_t modifier){
	setRate(modifierToRate(modifier));
}

void SpeedModifier::setSpeed(float speed){
	if(!(speed > 0.5f)){
		setRate(MinRate);
	}else if(speed >= 1.5f){
		setRate(MaxRate);
	}else{
		setRate(static_cast<Rate>(speed * RateScale + 0.5f));
	}
}

void SpeedModifier::setRate(Rate rate){
	if(rate < MinRate) rate = MinRate;
	if(rate > MaxRate) rate = MaxRate;
	requestedRate = rate;
}

SpeedModifier::Rate SpeedModifier::getRate() const{
	return requestedRate;
}

SpeedModifier::Rate SpeedModifier::getCurrentRate() const{
	return currentRate;
}

void SpeedModifier::nudgeRate(int32_t amount){
	const int64_t nudged = int64_t(requestedRate) + amount;
	if(nudged <= MinRate){
		setRate(MinRate);
	}else if(nudged >= MaxRate){
		setRate(MaxRate);
	}else{
		setRate(static_cast<Rate>(nudged));
	}
}

SpeedModifier::Rate SpeedModifier::modifierToRate(uint8_t modifier){
	if(modifier <= 127){
		return MinRate + (uint32_t(modifier) * (NeutralRate - MinRate) + 63) / 127;
	}
	return NeutralRate + (uint32_t(modifier - 127) * (MaxRate - NeutralRate) + 64) / 128;
}

void SpeedModifier::advanceRate(){
	static constexpr Rate step = RateScale / BUFFER_SAMPLES;
	if(currentRate < requestedRate){
		const Rate remaining = requestedRate - currentRate;
		currentRate += remaining < step ? remaining : step;
	}else if(currentRate > requestedRate){
		const Rate remaining = currentRate - requestedRate;
		currentRate -= remaining < step ? remaining : step;
	}
}

void SpeedModifier::fillBuffer(){
	while(source && dataBuffer->readAvailable() < BUFFER_SIZE * 2 && dataBuffer->writeAvailable() >= BUFFER_SIZE){
		size_t generated = source->generate(reinterpret_cast<int16_t*>(dataBuffer->writeData()));
		if(generated == 0) break;
		dataBuffer->writeMove(generated * BYTES_PER_SAMPLE * NUM_CHANNELS);
	}
}

void SpeedModifier::setSource(Source* source){
	SpeedModifier::source = source;
	dataBuffer->clear();
	sourcePosition = 0;
}

void SpeedModifier::reset(){
	dataBuffer->clear();
	remainder = 0;
}
