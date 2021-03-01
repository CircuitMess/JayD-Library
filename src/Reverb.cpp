#include "Reverb.h"

Reverb::Reverb(){

	delaySamples = 80.0f;
	decayFactor = 0.45f;

	delayAllPassSamples = 90;
	allPassDecay = 0.25f;

	mixPercent = 0;

	samplesBuffer = static_cast<int16_t *>(malloc(1024 * 50 * sizeof(int16_t)));
	if(!samplesBuffer){
		printf("Malloc ERROR");
	}

	for(int i = 0; i < 1024*50; ++i){
		samplesBuffer[i] = 0;
	}

	sampleBufferCnt = 0;
	startReverb = false;
}


void Reverb::combFilter(int16_t* sample, int numberOfSamples){

	/// Comb Filter: -> y[n] = x[n] + g·y[n−M]

	int delayComb1Samples = (int) ((float)(delaySamples * 44100.0f/1000.0f));
	int delayComb2Samples = (int) ((float)(delaySamples + 8.0f) * 44100.0f/1000.0f);
	int delayComb3Samples = (int) ((float)(delaySamples - 13.0f) * 44100.0f/1000.0f);
	int delayComb4Samples = (int) ((float)(delaySamples + 17.0f) * 44100.0f/1000.0f);

	combFilter1Output = static_cast<int32_t  *>(malloc(numberOfSamples * sizeof(int32_t)));
	combFilter2Output = static_cast<int32_t  *>(malloc(numberOfSamples * sizeof(int32_t)));
	combFilter3Output = static_cast<int32_t  *>(malloc(numberOfSamples * sizeof(int32_t)));
	combFilter4Output = static_cast<int32_t  *>(malloc(numberOfSamples * sizeof(int32_t)));


	for(int i=0; i < (numberOfSamples); i++){
		combFilter1Output[i] = combFilter2Output[i] = combFilter3Output[i] = combFilter4Output[i] = sample[i];
	}

	for(int i=0; i < (numberOfSamples - delayComb1Samples); i++){
		combFilter1Output[i+delayComb1Samples] += (int32_t)((float)combFilter1Output[i] * decayFactor);
	}
	for(int i=0; i < (numberOfSamples - delayComb2Samples); i++){
		combFilter2Output[i+delayComb2Samples] += (int32_t)((float)combFilter2Output[i] * (decayFactor-0.1313f));
	}
	for(int i=0; i < (numberOfSamples - delayComb3Samples); i++){
		combFilter3Output[i+delayComb3Samples] += (int32_t)((float)combFilter3Output[i] * (decayFactor-0.2743f));
	}
	for(int i=0; i < (numberOfSamples - delayComb4Samples); i++){
		combFilter4Output[i+delayComb4Samples] += (int32_t)((float)combFilter4Output[i] * (decayFactor-0.31f));
	}
}

void Reverb::allPassFilter(int32_t* samples, int numberOfSamples, uint8_t index){

	/// All-Pass Filter	-> y[n] = (−g·x[n]) + x[n−M] + (g·y[n−M])
	//
	// ‘M’ is the number of samples of delay.
	// This is calculated from the delay (τ) and the sampling rate as — M = Sampling rate * τ

	if(index == 1){

		allPassFilter1Output = static_cast<int32_t  *>(malloc(numberOfSamples * sizeof(int32_t)));

		for(int i = 0; i < (numberOfSamples); i++){

			allPassFilter1Output[i] = samples[i];
			//allPassFilter1Output[i+delayAllPassSamples] = (int32_t)((float)(-allPassDecay*(float)allPassFilter1Output[i+delayAllPassSamples])) + allPassFilter1Output[i] + (int32_t)((float)(allPassDecay*(float)allPassFilter1Output[i]));
			//printf("AllPass1In: %d\n", samples[i]);
			if(i - delayAllPassSamples >= 0){
				allPassFilter1Output[i] += (int32_t)((float)(-allPassDecay * (float)allPassFilter1Output[i - delayAllPassSamples]));
			}
			if(i - delayAllPassSamples >= 1){
				allPassFilter1Output[i] += (int32_t)((float)(allPassDecay * (float)allPassFilter1Output[i + 20 - delayAllPassSamples]));
			}
		}


		uint16_t maxAmp = pow(2,15)-1;
		int32_t treshold = maxAmp * 0.9;
		uint16_t window = maxAmp - treshold;

		for(int i=0; i<numberOfSamples; i++)
		{
			if(allPassFilter1Output[i] < 0){

				treshold = -treshold;
			}

			if(abs(allPassFilter1Output[i]) >= abs(treshold)){

				int32_t over = allPassFilter1Output[i] - treshold;
				float overC = (float)over/(float)window;
				allPassFilter1Output[i] = treshold + (float)over/(1.0/cos(overC*PI/2.0));
			}
			//printf("allPass1Out: %d\n", allPassFilter1Output[i]);
		}

	}
	else if(index == 2){

		allPassFilter2Output = static_cast<int32_t  *>(malloc(numberOfSamples * sizeof(int32_t)));

		for(int i = 0; i < (numberOfSamples); i++){

			allPassFilter2Output[i] = samples[i];
			//allPassFilter2Output[i+delayAllPassSamples] = (int32_t)((float)(-allPassDecay*(float)allPassFilter2Output[i+delayAllPassSamples])) + allPassFilter2Output[i] + (int32_t)((float)(allPassDecay*(float)allPassFilter2Output[i]));
			//printf("AllPass2In: %d\n", samples[i]);
			if(i - delayAllPassSamples >= 0){
				allPassFilter2Output[i] += (int32_t)((float)(-allPassDecay * (float)allPassFilter2Output[i - delayAllPassSamples]));
			}
			if(i - delayAllPassSamples >= 1){
				allPassFilter2Output[i] += (int32_t)((float)(allPassDecay * (float)allPassFilter2Output[i + 20 - delayAllPassSamples]));
			}
		}

		uint16_t maxAmp = pow(2,15)-1;
		int32_t treshold = maxAmp * 0.9;
		uint16_t window = maxAmp - treshold;

		for(int i=0; i<numberOfSamples; i++)
		{
			if(allPassFilter2Output[i] < 0){

				treshold = -treshold;
			}

			if(abs(allPassFilter2Output[i]) >= abs(treshold)){

				int32_t over = allPassFilter2Output[i] - treshold;
				float overC = (float)over/(float)window;
				allPassFilter2Output[i] = treshold + (float)over/(1.0/cos(overC*PI/2.0));
			}
			//printf("allPass2Out: %d\n", allPassFilter2Output[i]);
		}


	}else
		return;
}

void Reverb::signalProcessing(int16_t* sample, int numberOfSamples){

	combFilter(sample, numberOfSamples);

	int32_t* combParallelOut = static_cast<int32_t *>(malloc(numberOfSamples * sizeof(int32_t)));

	for(int i = 0; i < numberOfSamples; ++i){

		combParallelOut[i] = (float)(combFilter1Output[i] + combFilter2Output[i] + combFilter3Output[i] + combFilter4Output[i])/4.0f;
		//printf("Comb1Out: %d\n", combFilter1Output[i]);
	}

	int32_t* mixAudio = static_cast<int32_t *>(malloc(numberOfSamples * sizeof(int32_t)));

	for(int i=0; i<numberOfSamples; i++){
		mixAudio[i] = ((float)((100 - mixPercent) * sample[i])/100.0f + (float)(mixPercent * combParallelOut[i])/100.0f);
		//printf("mixAudio: %d\n", mixAudio[i]);
	}

	allPassFilter2Output = static_cast<int32_t  *>(malloc(numberOfSamples * sizeof(int32_t)));
	for(int i = 0; i < numberOfSamples; ++i){

		allPassFilter2Output[i] = mixAudio[i];
	}

	uint16_t maxAmp = pow(2,15)-1;
	int32_t treshold = maxAmp * 0.7;
	uint16_t window = maxAmp - treshold;

	for(int i=0; i<numberOfSamples; i++)
	{
		if(allPassFilter2Output[i] < 0){

			treshold = -treshold;
		}

		if(abs(allPassFilter2Output[i]) >= abs(treshold)){

			int32_t over = allPassFilter2Output[i] - treshold;
			float overC = (float)over/(float)window;
			allPassFilter2Output[i] = treshold + (float)over/(1.0/cos(overC*PI/2.0));
		}
		//printf("allPass2Out: %d\n", allPassFilter2Output[i]);
	}

	for(int i = 0; i < numberOfSamples; ++i){

		samplesBuffer[i] = mixAudio[i];
	}

	free(combParallelOut);

	//allPassFilter(mixAudio, numberOfSamples, 1);
	//allPassFilter(allPassFilter1Output, numberOfSamples, 2);

	free(mixAudio);
}

void Reverb::applyEffect(int16_t *inBuffer, int16_t *outBuffer, int numBytes){

	for(int i = 0; i < numBytes/2; ++i){
		samplesBuffer[i + (numBytes/2)*sampleBufferCnt] = inBuffer[i];
	}

	uint16_t maxAmp = pow(2,15)-1;
	int32_t treshold = maxAmp * 0.7;
	uint16_t window = maxAmp - treshold;

	if(startReverb){
		for(int i=0; i<numBytes/2; i++){

			outBuffer[i] = (1-((float)mixPercent/100.f))*(float)samplesBuffer[i + (numBytes/2)*sampleBufferCnt] + ((float)mixPercent/100.f)*(decayFactor*(float)samplesBuffer[i + (numBytes/2)*(sampleBufferCnt + 1)] + (decayFactor-0.13)*(float)samplesBuffer[i + (numBytes/2)*(sampleBufferCnt + 1)] + (decayFactor-0.27)*(float)samplesBuffer[i + (numBytes/2)*(sampleBufferCnt + 1)] + (decayFactor-0.31)*(float)samplesBuffer[i + (numBytes/2)*(sampleBufferCnt + 1)]);

			if(outBuffer[i] < 0){

				treshold = -treshold;
			}

			if(abs(outBuffer[i]) >= abs(treshold)){

				int32_t over = outBuffer[i] - treshold;
				float overC = (float)over/(float)window;
				outBuffer[i] = treshold + (float)over/(1.0/cos(overC*PI/2.0));
			}
		}
	}
	else{
		for(int i=0; i<numBytes/2; i++){

			outBuffer[i] = samplesBuffer[i + (numBytes/2)*sampleBufferCnt];
		}
	}


	sampleBufferCnt++;

	if(sampleBufferCnt >= 50){

		startReverb = true;
		sampleBufferCnt = 0;
	}

/*	free(combFilter1Output);
	free(combFilter2Output);
	free(combFilter3Output);
	free(combFilter4Output);
	free(allPassFilter1Output);
	free(allPassFilter2Output);*/
}

void Reverb::setIntensity(uint8_t intensity){

	mixPercent = ((float)intensity/236.0f) * 100;
	//decayFactor = ((float)intensity/236.0f) * 100;
}

Reverb::~Reverb() noexcept{

/*	free(allPassFilter1Output);
	free(allPassFilter2Output);*/

}
