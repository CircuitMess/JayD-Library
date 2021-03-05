#ifndef JAYD_SOURCEWAV_H
#define JAYD_SOURCEWAV_H

#include <Arduino.h>
#include <FS.h>
#include "Source.h"

class SourceWAV : public Source
{
public:
	SourceWAV();
	SourceWAV(fs::File *file);
	~SourceWAV();
	size_t generate(int16_t* outBuffer) override;
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

	void readHeader();

};


#endif