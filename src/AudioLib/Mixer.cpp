#include "Mixer.h"
#include "../AudioSetup.hpp"

//clipping wave to avoid overflows
int16_t clip(int32_t input){

    if (input > INT16_MAX) return INT16_MAX;
    if (input < INT16_MIN) return INT16_MIN;
    return input;
}


Mixer::Mixer()
{
}

Mixer::~Mixer()
{
	for(int16_t* buffer : bufferList){
		free(buffer);
	}
}

size_t Mixer::generate(int16_t *outBuffer){
	memset(outBuffer, 0, BUFFER_SIZE);
	std::fill(receivedSamples.begin(), receivedSamples.end(), 0);

	for(uint8_t i = 0; i < sourceList.size(); i++){
		if(pauseList[i]) continue;
		Generator* generator = sourceList[i];
		int16_t* buffer = bufferList[i];
		if(generator != nullptr && buffer != nullptr){
			receivedSamples[i] = generator->generate(buffer);
		}
	}

	for(uint16_t i = 0; i < BUFFER_SAMPLES*NUM_CHANNELS; i++){
		int32_t wave = 0;
		for(uint8_t j = 0; j < sourceList.size(); j++){
			if(pauseList[j]) continue;
			if(bufferList[j] == nullptr || receivedSamples[j] <= i/NUM_CHANNELS) continue;

			if(sourceList.size() == 2){
				wave += bufferList[j][i] * (float)((j == 1 ? (float)(mixRatio) : (float)(255.0 - mixRatio))/255.0); //use the mixer if only 2 tracks found
			}else{
				wave += bufferList[j][i] * (float)(1.0/(float)(sourceList.size())); // evenly distribute all tracks
			}
		}
		outBuffer[i] = clip(wave);
	}
	size_t longestBuffer = receivedSamples.empty()
		? 0
		: *std::max_element(receivedSamples.begin(), receivedSamples.end());

	if(longestBuffer == 0){
		// SD-backed decoders can briefly underflow; silence this block without
		// turning a recoverable read delay into a persistent deck pause.
		return BUFFER_SAMPLES;
	}

	return longestBuffer;
}

int Mixer::available(){
	int available = 0;
	for(auto & i : sourceList){
		available = max(available, i->available());
	}
	return available;
}

void Mixer::addSource(Generator* generator){
	sourceList.push_back(generator);

	int16_t* buffer;
	if(psramFound()){
		buffer = (int16_t*) ps_calloc(BUFFER_SIZE, sizeof(byte));
	}else{
		buffer = (int16_t*) calloc(BUFFER_SIZE, sizeof(byte));
	}

	bufferList.push_back(buffer);
	receivedSamples.push_back(0);
	pauseList.push_back(false);
}

void Mixer::setMixRatio(uint8_t ratio){
	mixRatio = ratio;
}

uint8_t Mixer::getMixRatio(){
	return uint8_t(mixRatio);
}

Generator* Mixer::getSource(size_t index){
	return sourceList[index];
}

void Mixer::setSource(size_t index, Generator* source){
	if(index >= sourceList.size()) return;
	sourceList[index] = source;
}

void Mixer::pauseChannel(uint8_t channel) {
	if(channel >= pauseList.size()) return;
	pauseList[channel] = true;
}

void Mixer::resumeChannel(uint8_t channel) {
	if(channel >= pauseList.size()) return;
	pauseList[channel] = false;
}

bool Mixer::isChannelPaused(uint8_t channel){
	if(channel >= pauseList.size()) return false;
	return pauseList[channel];
}
