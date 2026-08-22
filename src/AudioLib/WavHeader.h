#ifndef JAYD_LIBRARY_WAVHEADER_H
#define JAYD_LIBRARY_WAVHEADER_H

#include <stdint.h>
#include <string.h>

struct WavHeader {
	char RIFF[4];
	uint32_t chunkSize;
	char WAVE[4];
	char fmt[4];
	uint32_t fmtSize;
	uint16_t audioFormat;
	uint16_t numChannels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
	char data[4];
	uint32_t dataSize;
};

static_assert(sizeof(WavHeader) == 44, "PCM WAV header must be 44 bytes");

inline WavHeader makeWavHeader(
		uint32_t dataSize,
		uint16_t channels,
		uint32_t sampleRate,
		uint16_t bytesPerSample
){
	WavHeader header = {};
	memcpy(header.RIFF, "RIFF", 4);
	header.chunkSize = dataSize + 36;
	memcpy(header.WAVE, "WAVE", 4);
	memcpy(header.fmt, "fmt ", 4);
	header.fmtSize = 16;
	header.audioFormat = 1;
	header.numChannels = channels;
	header.sampleRate = sampleRate;
	header.byteRate = sampleRate * channels * bytesPerSample;
	header.blockAlign = channels * bytesPerSample;
	header.bitsPerSample = bytesPerSample * 8;
	memcpy(header.data, "data", 4);
	header.dataSize = dataSize;
	return header;
}

#endif //JAYD_LIBRARY_WAVHEADER_H
