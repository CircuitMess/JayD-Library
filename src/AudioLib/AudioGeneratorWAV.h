#ifndef JAYD_AUDIOGENERATORWAV_H
#define JAYD_AUDIOGENERATORWAV_H

#include <Arduino.h>
#include <FS.h>
#include "AudioSource.h"

class AudioGeneratorWAV : public AudioSource
{
public:
	AudioGeneratorWAV(fs::File *file);
	~AudioGeneratorWAV();
	int generate(int16_t* outBuffer) override;

	int getBitsPerSample() override;
	int getSampleRate() override;
	int getChannels() override;

private:
	fs::File *file;

	uint8_t channels;
	uint32_t sampleRate;
	uint8_t bitsPerSample;

	void readHeader();

};


#endif