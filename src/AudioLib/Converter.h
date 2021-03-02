#ifndef JAYD_CONVERTER_H
#define JAYD_CONVERTER_H

#include "Generator.h"
#include "Source.h"
#include "r8brain-free-src/r8bbase.h"
#include "r8brain-free-src/CDSPResampler.h"

class Converter : public Generator
{
public:
	Converter(Source* source);
	~Converter();
	int generate(int16_t* outBuffer) override;

private:
	Source* source;
	int16_t* conversionBuffer;

	r8b::CFixedBuffer< double > InBufs[2]; //max 2 channels in audio files
	r8b::CPtrKeeper< r8b::CDSPResampler24* > Resamps[2];
};



#endif