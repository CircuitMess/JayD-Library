#include <assert.h>
#include <string.h>
#include "../src/AudioLib/WavHeader.h"

int main(){
	const WavHeader empty = makeWavHeader(0, 1, 24000, 2);
	assert(sizeof(empty) == 44);
	assert(memcmp(empty.RIFF, "RIFF", 4) == 0);
	assert(memcmp(empty.WAVE, "WAVE", 4) == 0);
	assert(memcmp(empty.fmt, "fmt ", 4) == 0);
	assert(memcmp(empty.data, "data", 4) == 0);
	assert(empty.chunkSize == 36);
	assert(empty.dataSize == 0);
	assert(empty.byteRate == 48000);
	assert(empty.blockAlign == 2);
	assert(empty.bitsPerSample == 16);

	const WavHeader finalized = makeWavHeader(4096, 2, 44100, 2);
	unsigned char fileHeader[sizeof(WavHeader)] = {};
	memcpy(fileHeader, &empty, sizeof(empty));
	memcpy(fileHeader, &finalized, sizeof(finalized));
	WavHeader rewritten = {};
	memcpy(&rewritten, fileHeader, sizeof(rewritten));
	assert(rewritten.chunkSize == 4132);
	assert(rewritten.dataSize == 4096);
	assert(rewritten.byteRate == 176400);
	assert(rewritten.blockAlign == 4);
	return 0;
}
