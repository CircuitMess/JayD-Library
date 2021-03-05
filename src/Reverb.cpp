#include "Reverb.h"

Reverb::Reverb(){

	decayFactor = 0.55f;
	mixPercent = 0;


	Serial.println(ESP.getFreeHeap());

	samplesBuffer = static_cast<float *>(calloc(1024 * EXTENDED_BUFFER, sizeof(float)));
	if(!samplesBuffer){
		printf("Allocating ERROR");
		Serial.flush();
	}

	Serial.println(ESP.getFreeHeap());

	sampleBufferCnt = 0;
	startReverb = false;

	maxAmp = pow(2,15)-1;
	treshold = maxAmp * 0.8;
}


int32_t Reverb::signalProcessing(uint32_t index, uint32_t delay0, uint32_t delay1, uint32_t delay2, uint32_t delay3){

	int32_t acc;

	acc = (1-((float)mixPercent/100.f)) * (float)samplesBuffer[index] +
			((float)mixPercent/100.f) * (
					decayFactor * (float)samplesBuffer[delay0] +
					(decayFactor-0.13) * (float)samplesBuffer[delay1] +
					(decayFactor-0.27) * (float)samplesBuffer[delay2] +
					(decayFactor-0.31) * (float)samplesBuffer[delay3]
					);

	if(acc < 0){

		treshold = -treshold;
	}

	if(abs(acc) >= abs(treshold)){

		uint16_t window = maxAmp - treshold;
		int32_t over = acc - treshold;
		float overC = (float)over/(float)window;
		acc = treshold + (float)over/(1.0/cos(overC*PI/2.0));
	}

	return (int16_t)acc;
}

void Reverb::applyEffect(int16_t *inBuffer, int16_t *outBuffer, int numBytes){

	for(int i = 0; i < numBytes/2; ++i){
		samplesBuffer[i + (numBytes/2)*sampleBufferCnt] = inBuffer[i];
	}

	float lastIn = 0, lastOut = 0;
	float c = 0.15;

	if(startReverb){
		for(int i=0; i<numBytes/2; i++){

			outBuffer[i] = signalProcessing(
					i + (numBytes/2)*(sampleBufferCnt),
					i + (numBytes/2)*((sampleBufferCnt + 1) % 30),
					i + (numBytes/2)*((sampleBufferCnt + 2) % 30),
					i + (numBytes/2)*((sampleBufferCnt + 3) % 30),
					i + (numBytes/2)*((sampleBufferCnt + 4) % 30)
					);

			float lastInTemp = outBuffer[i];

			outBuffer[i] = - c * (float)outBuffer[i] + lastIn + c * lastOut;
			lastIn = lastInTemp;
			lastOut = outBuffer[i];

		}
	}
	else{
		for(int i=0; i<numBytes/2; i++){

			outBuffer[i] = samplesBuffer[i + (numBytes/2)*sampleBufferCnt];
		}
	}


	if(++sampleBufferCnt >= 30){

		startReverb = true;
		sampleBufferCnt = 0;
	}

}

void Reverb::setIntensity(uint8_t intensity){

	mixPercent = ((float)intensity/236.0f) * 50;
}

Reverb::~Reverb() noexcept{

	free(samplesBuffer);
}
