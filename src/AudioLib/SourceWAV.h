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

	uint16_t getDuration() override;
	uint16_t getElapsed() override;
	void seek(uint16_t time, fs::SeekMode mode) override;

	void open(fs::File *file);

private:
	fs::File *file;

	size_t dataSize;
	size_t readData;
	bool readHeader();

	uint8_t* fileBuffer = nullptr;
	uint8_t fbPtr = 0;
};


#endif