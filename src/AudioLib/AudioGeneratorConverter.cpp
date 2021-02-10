#include "AudioGeneratorConverter.h"

AudioGeneratorConverter::AudioGeneratorConverter(AudioSource* source) : source(source)
{
	// size_t buffSize = 800;
	// conversionBuffer = (int16_t*)calloc(buffSize, sizeof(int16_t));
		// if(source->isStereo()){
	// 	buffSize*=2;
	// }

	const int InBufCapacity = 800; //taking in 800 samples at a time
	for( uint8_t i = 0; i < source->getChannels(); i++ )
	{
		InBufs[ i ].alloc( InBufCapacity );
		Resamps[ i ] = new r8b::CDSPResampler24( source->getSampleRate(), outSampleRate, InBufCapacity );
	}
}

AudioGeneratorConverter::~AudioGeneratorConverter()
{
}

int AudioGeneratorConverter::generate(int16_t* outBuffer)
{
	int readSamples = source->generate((int16_t*)(InBufs[0].getPtr()));

	//separate into 2 buffers if necessary
	if(source->getChannels() == 2){
		if(source->getBitsPerSample() == 16){
			for(int32_t i = 0; i < readSamples/2; i++){
				((int16_t*)(InBufs[1].getPtr()))[i] = ((int16_t*)(InBufs[0].getPtr()))[i + 1];
				if(i > 0){
					memmove(((int16_t*)(InBufs[0].getPtr())) + i - 1, ((int16_t*)(InBufs[0].getPtr())) + i, (readSamples - i*2)*sizeof(int16_t));
				}
			}
		}else if(source->getBitsPerSample() == 32){
			for(int32_t i = 0; i < readSamples/2; i++){
				((int32_t*)(InBufs[1].getPtr()))[i] = ((int32_t*)(InBufs[0].getPtr()))[i + 1];
				if(i > 0){
					memmove(((int32_t*)(InBufs[0].getPtr())) + i - 1, ((int32_t*)(InBufs[0].getPtr())) + i, (readSamples - i*2)*sizeof(int32_t));
				}
			}
		}
		readSamples/=2;
	}
	
	//resample to 44100Hz
	int WriteCount;
	if(source->getSampleRate() != outSampleRate){
		double* opp[ source->getChannels() ];

		for(int i = 0; i < source->getChannels(); i++ )
		{
			WriteCount = Resamps[ i ] -> process(InBufs[ i ], readSamples, opp[ i ]);
		}
	}

	// if(source->getBitsPerSample() == 32){
		
	// 	for(int i = 0; i < readSamples; i++){
	// 		*(((int32_t*)InBufs[0]) + i) = *(((int32_t*)InBufs[0]) + i) >> 16;
	// 	}
	// }
}
