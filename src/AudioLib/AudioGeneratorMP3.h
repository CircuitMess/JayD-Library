#ifndef JAYD_AUDIOGENERATORMP3_H
#define JAYD_AUDIOGENERATORMP3_H

#include <Arduino.h>
#include <FS.h>
#include "AudioSource.h"

class AudioGeneratorMP3 : public AudioSource
{
public:
	AudioGeneratorMP3();
	AudioGeneratorMP3(fs::File *file);
	~AudioGeneratorMP3();
	int generate(int16_t* outBuffer) override;
	int available() override;

	int getBitsPerSample() override;
	int getSampleRate() override;
	int getChannels() override;

	void open(fs::File *file);

private:
	fs::File *file;

	uint8_t channels;
	uint32_t sampleRate;
	uint8_t bitsPerSample;
};


#endif