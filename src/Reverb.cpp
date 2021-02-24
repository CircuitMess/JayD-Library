//
// Created by Domagoj on 19/02/2021.
//

#include "Reverb.h"

Reverb::Reverb(){

	combFilter1Output = static_cast<int32_t  *>(malloc(1024 * sizeof(int32_t)));
	combFilter2Output = static_cast<int32_t  *>(malloc(1024 * sizeof(int32_t)));
	combFilter3Output = static_cast<int32_t  *>(malloc(1024 * sizeof(int32_t)));
	combFilter4Output = static_cast<int32_t  *>(malloc(1024 * sizeof(int32_t)));
	allPassFilter1Output = static_cast<int32_t  *>(malloc(1024 * sizeof(int32_t)));
	allPassFilter2Output = static_cast<int32_t  *>(malloc(1024 * sizeof(int32_t)));
}


void Reverb::combFilter(int16_t* sample, int numberOfSamples){

	/// Comb Filter: -> y[n] = x[n] + g·y[n−M]

	int delayComb1Samples = (int) ((float)delaySamples);
	int delayComb2Samples = (int) ((float)(delaySamples - 11.73f));
	int delayComb3Samples = (int) ((float)(delaySamples + 19.31f));
	int delayComb4Samples = (int) ((float)(delaySamples - 7.97f));

	if(sizeof (combFilter1Output) < numberOfSamples * sizeof (int32_t)){
		combFilter1Output = static_cast<int32_t *>(realloc(combFilter1Output, numberOfSamples * sizeof(int32_t)));
	}
	if(sizeof (combFilter2Output) < numberOfSamples * sizeof (int32_t)){
		combFilter2Output = static_cast<int32_t *>(realloc(combFilter2Output, numberOfSamples * sizeof(int32_t)));
	}
	if(sizeof (combFilter3Output) < numberOfSamples * sizeof (int32_t)){
		combFilter3Output = static_cast<int32_t *>(realloc(combFilter3Output, numberOfSamples * sizeof(int32_t)));
	}
	if(sizeof (combFilter4Output) < numberOfSamples * sizeof (int32_t)){
		combFilter4Output = static_cast<int32_t *>(realloc(combFilter4Output, numberOfSamples * sizeof(int32_t)));
	}

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
	printf("comb_finished\n");
}

void Reverb::allPassFilter(int32_t* samples, int numberOfSamples, uint8_t index){

	/// All-Pass Filter	-> y[n] = (−g·x[n]) + x[n−M] + (g·y[n−M])
	//
	// ‘M’ is the number of samples of delay.
	// This is calculated from the delay (τ) and the sampling rate as — M = Sampling rate * τ

	int delayAllPassSamples = (int) (float)89.27f;
	int16_t M = SAMPLE_RATE*delayAllPassSamples;
	float allPassDecay = 0.131f;

	if(sizeof (allPassFilter1Output) < numberOfSamples * sizeof (int32_t)){
		allPassFilter1Output = static_cast<int32_t *>(realloc(allPassFilter1Output, numberOfSamples * sizeof(int32_t)));
	}
	if(sizeof (allPassFilter2Output) < numberOfSamples * sizeof (int32_t)){
		allPassFilter2Output = static_cast<int32_t *>(realloc(allPassFilter2Output, numberOfSamples * sizeof(int32_t)));
	}

	printf("Index: %d\n", index);

	if(index == 1){

		for(int i = 0; i < (numberOfSamples - delayAllPassSamples); i++){

			allPassFilter1Output[i] = samples[i];
			allPassFilter1Output[i+delayAllPassSamples] = -allPassDecay*allPassFilter1Output[i+delayAllPassSamples] + allPassFilter1Output[i] + allPassDecay*allPassFilter1Output[i];
			//printf("AllPass1Out: %d\n", allPassFilter1Output[i+delayAllPassSamples]);
			/*if(i - delayAllPassSamples >= 0){
				allPassFilter1Output[i] += (int32_t)(-allPassDecay * (float)allPassFilter1Output[i - delayAllPassSamples]);
			}
			if(i - delayAllPassSamples >= 1){
				allPassFilter1Output[i] += (int32_t)(allPassDecay * (float)allPassFilter1Output[i + 20 - delayAllPassSamples]);
			}*/
		}

		int32_t value = allPassFilter1Output[0];
		int32_t max = 0.0f;

		for(int i=0; i < numberOfSamples; i++)
		{
			if(abs(allPassFilter1Output[i]) > max)
				max = abs(allPassFilter1Output[i]);
		}

		for(int i=0; i<numberOfSamples; i++)
		{
			int32_t currentValue = allPassFilter1Output[i];
			value = ((value + (currentValue - value))/max);

			allPassFilter1Output[i] = value;
		}
	}
	else if(index == 2){

		for(int i = 0; i < (numberOfSamples - delayAllPassSamples); i++){

			allPassFilter2Output[i] = samples[i];
			allPassFilter2Output[i+delayAllPassSamples] = -allPassDecay*allPassFilter2Output[i+delayAllPassSamples] + allPassFilter2Output[i] + allPassDecay*allPassFilter2Output[i + 20];
			/*if(i - delayAllPassSamples >= 0){
				allPassFilter2Output[i] += (int32_t)(-allPassDecay * (float)allPassFilter2Output[i - delayAllPassSamples]);
			}
			if(i - delayAllPassSamples >= 1){
				allPassFilter2Output[i] += (int32_t)(allPassDecay * (float)allPassFilter2Output[i + 20 - delayAllPassSamples]);
			}*/
		}

		int32_t value = allPassFilter2Output[0];
		int32_t max = 0.0f;

		for(int i=0; i < numberOfSamples; i++)
		{
			if(abs(allPassFilter2Output[i]) > max)
				max = abs(allPassFilter2Output[i]);
		}

		for(int i=0; i<numberOfSamples; i++)
		{
			int32_t currentValue = allPassFilter2Output[i];
			value = ((value + (currentValue - value))/max);

			allPassFilter2Output[i] = value;
		}

	}else
		return;
}

void Reverb::signalProcessing(int16_t* sample, int numberOfSamples){

	combFilter(sample, numberOfSamples);

	int32_t* combParallelOut = static_cast<int32_t *>(malloc(numberOfSamples * sizeof(int32_t)));

	for(int i = 0; i < numberOfSamples; ++i){

		combParallelOut[i] = combFilter1Output[i] + combFilter2Output[i] + combFilter3Output[i] + combFilter4Output[i];
		printf("CombParallelOut: %d\n", combParallelOut[i]);
		printf("Comb1Out: %d\n", combFilter1Output[i]);
		printf("Comb2Out: %d\n", combFilter2Output[i]);
		printf("Comb3Out: %d\n", combFilter3Output[i]);
		printf("Comb4Out: %d\n", combFilter4Output[i]);

	}

	int32_t* mixAudio = static_cast<int32_t *>(malloc(numberOfSamples * sizeof(int32_t)));
	uint8_t mixPercent = 50;

	for(int i=0; i<numberOfSamples; i++){
		mixAudio[i] = ((100 - mixPercent) * sample[i]) + (mixPercent * combParallelOut[i]);
		printf("mixAudioOut: %d\n", mixAudio[i]);
	}

	free(combParallelOut);

	allPassFilter(mixAudio, numberOfSamples, 1);
	allPassFilter(allPassFilter1Output, numberOfSamples, 2);

	free(mixAudio);
}

void Reverb::applyEffect(int16_t *inBuffer, int16_t *outBuffer, int numBytes){

	signalProcessing(inBuffer , numBytes/2);

	outBuffer = reinterpret_cast<int16_t *>(allPassFilter2Output);

	for(int i = 0; i < numBytes/2; ++i){
		printf("Out: %d\n", allPassFilter2Output[i]);
	}
}

void Reverb::setIntensity(uint8_t intensity){

}

Reverb::~Reverb() noexcept{

	free(combFilter1Output);
	free(combFilter2Output);
	free(combFilter3Output);
	free(combFilter4Output);

	free(allPassFilter1Output);
	free(allPassFilter2Output);
}
