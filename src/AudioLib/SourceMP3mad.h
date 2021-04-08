#ifndef JAYD_SOURCEMP3MAD_H
#define JAYD_SOURCEMP3MAD_H

#include <Arduino.h>
#include <FS.h>
#include "Source.h"
#include "Decoder/ID3Parser.h"
#include "Decoder/mad/mad.h"
#include "../AudioSetup.hpp"
#include <Buffer/FSBuffer.h>
#include <Buffer/DataBuffer.h>

// 2106
#define MP3_BUFFERSIZE 2106 * 3
#define DATA_BUFFERSIZE (1156 * 2)

class SourceMP3mad : public Source
{
public:
	SourceMP3mad(fs::File& file);
	SourceMP3mad(const String& path);
	~SourceMP3mad();
	size_t generate(int16_t* outBuffer) override;
	int available() override;

	void open(fs::File& file);
	void open(const String& path);

	void close() override;

	uint16_t getDuration() override;
	uint16_t getElapsed() override;

	const ID3Metadata& getMetadata() const;

	void seek(uint16_t time, fs::SeekMode mode) override;

private:
	fs::File file;

	FSBuffer* input = nullptr;
	DataBuffer* output = nullptr;

	ID3Metadata metadata;

	size_t dataStart = 0;

	mad_stream* stream = nullptr;
	mad_frame* frame = nullptr;
	mad_synth* synth = nullptr;
	size_t samplesAvailable = 0;
	size_t samplesRead = 0;

	void decode();

	void processSynth();

	fs::File openUnicodePath(const char* path);
};


#endif