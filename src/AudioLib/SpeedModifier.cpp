#include "SpeedModifier.h"
#include "../AudioSetup.hpp"

SpeedModifier::SpeedModifier(Source* source) : source(source){

	outBuff = new DataBuffer(BUFFER_SIZE);

	modifier = 100;
}

size_t SpeedModifier::generate(int16_t *outBuffer){

	size_t outBufferSize = 0;
	uint16_t numSamplesWritten = 0;

	if(modifier < 100){

		uint8_t sMod = 100 / modifier;
		///float sModF = 100.0f / (float)modifier;

		if(source->available()){

			numSamplesWritten = source->generate((int16_t *) outBuff->writeData());
		}

		if(!outBuff->writeMove(numSamplesWritten * 2)){
			printf("Error moving write cursor!\n");
		}

		if(outBuff->readAvailable() >= BUFFER_SIZE/sMod){
			outBufferSize = BUFFER_SIZE/sMod;
		}
		else{
			outBufferSize = outBuff->readAvailable();
		}

		auto* temp = (int16_t *)outBuff->readData();

		for(int i = 0, j=0; i < outBufferSize/2 * sMod; i++){

			if(i % sMod == 0){
				outBuffer[i] = temp[j++];
			}
			else{
				outBuffer[i] = (temp[j-1])/2 + (temp[j])/2;
			}

			///outBuffer[i] = temp[(uint16_t)((float)i*sModF)];
		}

		if(!outBuff->readMove(outBufferSize*sMod)){
			printf("Error moving read cursor!\n");
		}

		return outBufferSize/2 * sMod;
	}
	else if(modifier >= 100){

		float sModF = (float)modifier / 100.0f;

		if(source->available()){

			numSamplesWritten = source->generate((int16_t *) outBuff->writeData());
		}

		if(!outBuff->writeMove(numSamplesWritten * 2)){
			printf("Error moving write cursor!\n");
		}

		outBufferSize = outBuff->readAvailable();

		auto* temp = (int16_t *)outBuff->readData();

		for(int i = 0; (uint16_t)((float)i*sModF) < outBufferSize/2; ++i){

			outBuffer[i] = temp[(uint16_t)((float)i*sModF)];
		}

		if(!outBuff->readMove(outBuff->readAvailable())){
			printf("Error moving read cursor!\n");
		}

		return outBufferSize/(uint16_t)((float)(2.0f*sModF));
	}
	else{

		/*uint16_t numSamplesWritten;

		if(source->available()){

			numSamplesWritten = source->generate((int16_t *) outBuff->writeData());
		}

		if(!outBuff->writeMove(numSamplesWritten * 2)){
			Serial.println("writeMove error");
		}

		outBufferSize = outBuff->readAvailable();

		memcpy(outBuffer, (int16_t *)outBuff->readData(), outBufferSize);

		if(!outBuff->readMove(outBuff->readAvailable())){
			Serial.println("readMove error");
		}

		return outBufferSize/2;*/
	}
}

int SpeedModifier::available(){

	if(source == nullptr || outBuff == nullptr || modifier == 0){

		printf("Speed modifier not available!\n");
		return 0;
	}
}

void SpeedModifier::setModifier(uint16_t _modifier){

	modifier = _modifier;
}

SpeedModifier::~SpeedModifier() noexcept{

	delete outBuff;
}
