#include "../src/AudioLib/ADTSTiming.h"
#include <assert.h>
#include <limits.h>
#include <string.h>

static void makeFrame(uint8_t* frame, size_t size, uint8_t sampleRateIndex = 6){
	assert(size >= 7 && size <= 8191);
	memset(frame, 0, size);
	frame[0] = 0xff;
	frame[1] = 0xf1;
	frame[2] = uint8_t((1u << 6) | (sampleRateIndex << 2));
	frame[3] = uint8_t((1u << 6) | ((size >> 11) & 0x03));
	frame[4] = uint8_t(size >> 3);
	frame[5] = uint8_t((size & 0x07) << 5);
}

int main(){
	static_assert(sizeof(ADTSTiming::FrameIndexEntry) == 8, "seek entry size changed");

	uint8_t cbr[20];
	uint8_t vbr[33];
	makeFrame(cbr, sizeof(cbr));
	makeFrame(vbr, sizeof(vbr));

	ADTSTiming::Header header;
	assert(ADTSTiming::parseFrame(cbr, sizeof(cbr), header) == ADTSTiming::VALID);
	assert(header.frameLength == sizeof(cbr));
	assert(header.sampleRate == 24000);
	assert(header.channels == 1);
	assert(header.sourceFrames == 1024);
	assert(ADTSTiming::parseFrame(vbr, sizeof(vbr), header) == ADTSTiming::VALID);
	assert(header.frameLength == sizeof(vbr));

	cbr[0] = 0xfe;
	assert(ADTSTiming::parseFrame(cbr, sizeof(cbr), header) == ADTSTiming::INVALID);
	makeFrame(cbr, sizeof(cbr));
	assert(ADTSTiming::parseFrame(cbr, sizeof(cbr) - 1, header) == ADTSTiming::NEED_MORE);

	uint64_t frames;
	assert(ADTSTiming::secondsToFrames(60, 24000, frames) && frames == 1440000);
	assert(!ADTSTiming::secondsToFrames(UINT64_MAX, 24000, frames));

	const ADTSTiming::FrameIndexEntry index[] = {
			{ 10, 0 },
			{ 30, 1024 },
			{ 63, 2048 }
	};
	assert(ADTSTiming::findPreceding(index, 3, 0) == 0);
	assert(ADTSTiming::findPreceding(index, 3, 1023) == 0);
	assert(ADTSTiming::findPreceding(index, 3, 1024) == 1);
	assert(ADTSTiming::findPreceding(index, 3, UINT64_MAX) == 2);

	size_t required = 0;
	assert(ADTSTiming::requiredDecodeBytes(4, 8192, 32768, required));
	assert(required == 32768);
	assert(!ADTSTiming::requiredDecodeBytes(4, 8192, 32767, required));
	assert(!ADTSTiming::requiredDecodeBytes(SIZE_MAX, 8192, SIZE_MAX, required));

	ADTSTiming::EofNotification eof;
	assert(eof.take());       // first non-repeat EOF generate
	assert(!eof.take());      // repeated generate after the same EOF
	eof.reset();              // repeat rewind
	assert(eof.take());
	eof.reset();              // seek after EOF
	assert(eof.take());
	eof.reset();              // reopen
	assert(eof.take());
}
