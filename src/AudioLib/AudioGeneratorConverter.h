#ifndef JAYD_AUDIOGENERATORCONVERTER_H
#define JAYD_AUDIOGENERATORCONVERTER_H

#include "AudioGenerator.h"
#include "AudioSource.h"
#include "r8brain-free-src/r8bbase.h"
#include "r8brain-free-src/CDSPResampler.h"

class AudioGeneratorConverter : public AudioGenerator
{
public:
	AudioGeneratorConverter(AudioSource* source);
	~AudioGeneratorConverter();
	int generate(int16_t* outBuffer) override;

private:
	AudioSource* source;
	int16_t* conversionBuffer;

	r8b::CFixedBuffer< double > InBufs[2]; //max 2 channels in audio files
	r8b::CPtrKeeper< r8b::CDSPResampler24* > Resamps[2];

	const int outSampleRate = 44100;

};



#endif