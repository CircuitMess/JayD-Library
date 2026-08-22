#ifndef JAYD_ADTSTIMING_H
#define JAYD_ADTSTIMING_H

#include <stddef.h>
#include <stdint.h>

namespace ADTSTiming {

enum ParseResult : uint8_t {
	INVALID,
	NEED_MORE,
	VALID
};

struct Header {
	uint16_t frameLength;
	uint16_t sourceFrames;
	uint32_t sampleRate;
	uint8_t channels;
	uint8_t headerLength;
};

struct FrameIndexEntry {
	uint32_t offset;
	uint32_t sourceFrame;
};

class EofNotification {
public:
	bool take(){
		if(notified) return false;
		notified = true;
		return true;
	}

	void reset(){
		notified = false;
	}

private:
	bool notified = false;
};

inline bool requiredDecodeBytes(size_t rawBlocks, size_t bytesPerBlock,
								size_t capacity, size_t& required){
	if(rawBlocks == 0 || bytesPerBlock == 0 ||
	   rawBlocks > SIZE_MAX / bytesPerBlock){
		return false;
	}
	required = rawBlocks * bytesPerBlock;
	return required <= capacity;
}

inline ParseResult parseHeader(const uint8_t* data, size_t size, Header& header){
	static const uint32_t sampleRates[] = {
			96000, 88200, 64000, 48000, 44100, 32000, 24000,
			22050, 16000, 12000, 11025, 8000, 7350
	};

	if(size < 7) return NEED_MORE;
	if(data[0] != 0xff || (data[1] & 0xf6) != 0xf0) return INVALID;

	const uint8_t profile = (data[2] >> 6) & 0x03;
	const uint8_t sampleRateIndex = (data[2] >> 2) & 0x0f;
	if(profile == 3 || sampleRateIndex >= sizeof(sampleRates) / sizeof(sampleRates[0])) return INVALID;

	header.headerLength = (data[1] & 0x01) ? 7 : 9;
	if(size < header.headerLength) return NEED_MORE;

	header.frameLength = uint16_t((uint16_t(data[3] & 0x03) << 11) |
								  (uint16_t(data[4]) << 3) |
								  (data[5] >> 5));
	if(header.frameLength < header.headerLength) return INVALID;

	header.sampleRate = sampleRates[sampleRateIndex];
	header.channels = uint8_t(((data[2] & 0x01) << 2) | (data[3] >> 6));
	header.sourceFrames = uint16_t(1024u * ((data[6] & 0x03) + 1u));
	return VALID;
}

inline ParseResult parseFrame(const uint8_t* data, size_t size, Header& header){
	const ParseResult result = parseHeader(data, size, header);
	if(result != VALID) return result;
	return header.frameLength <= size ? VALID : NEED_MORE;
}

inline bool secondsToFrames(uint64_t seconds, uint32_t sampleRate, uint64_t& frames){
	if(sampleRate == 0 || seconds > UINT64_MAX / sampleRate) return false;
	frames = seconds * sampleRate;
	return true;
}

inline size_t findPreceding(const FrameIndexEntry* entries, size_t count, uint64_t target){
	size_t low = 0;
	size_t high = count;
	while(low < high){
		const size_t middle = low + (high - low) / 2;
		if(entries[middle].sourceFrame <= target){
			low = middle + 1;
		}else{
			high = middle;
		}
	}
	return low == 0 ? 0 : low - 1;
}

}

#endif
