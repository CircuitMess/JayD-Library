#ifndef JAYD_SOURCEAAC_H
#define JAYD_SOURCEAAC_H

#include <Arduino.h>
#include "../AudioSetup.hpp"
#include <FS.h>
#include "Source.h"
#include "../Services/SDScheduler.h"
#include "Decoder/libhelix-aac/aacdec.h"
#include <Buffer/RingBuffer.h>
#include <Sync/Mutex.h>
#include <Util/Task.h>
#include <aacenc_lib.h>
#include <aacdecoder_lib.h>
#include <Buffer/DataBuffer.h>
#include "ADTSTiming.h"

class SourceAAC : public Source
{
public:
	SourceAAC();
	SourceAAC(fs::File file);
	~SourceAAC();
	size_t generate(int16_t* outBuffer) override;
	int available() override;

	uint16_t getDuration() override;
	uint16_t getElapsed() override;
	void seek(uint16_t time, fs::SeekMode mode) override;

	uint64_t getDurationSourceFrames() const;
	uint64_t getElapsedSourceFrames() const;
	uint32_t getSourceSampleRate() const;
	uint8_t getSourceChannels() const;
	bool seekSourceFrame(uint64_t frame);

	enum FrameIndexQuality : uint8_t {
		INDEX_UNAVAILABLE,
		INDEX_PENDING,
		INDEX_PARTIAL,
		INDEX_COMPLETE
	};
	FrameIndexQuality getFrameIndexQuality() const;

	void open(fs::File file);

	void close() override;

	void setVolume(uint8_t volume);

	void setRepeat(bool repeat);
	void setSongDoneCallback(void (*callback)());
	bool isReadReady() const;

private:
	fs::File file;

	float volume = 1.0f;

	RingBuffer readBuffer;
	bool readJobPending = false;

	DataBuffer fillBuffer;
	DataBuffer dataBuffer;
	void refill();
	bool prepareNextFrame(ADTSTiming::Header& header);

	HAACDecoder hAACDecoder = nullptr;

	SDResult* readResult = nullptr;
	void addReadJob(bool full = false);
	void processReadJob(bool wait = false);
	void resetDecoding();
	void startFrameIndex();
	void buildFrameIndex(Task* task);
	void freeFrameIndex();
	static void indexThread(Task* task);

	ADTSTiming::FrameIndexEntry* frameIndex = nullptr;
	size_t frameIndexCount = 0;
	size_t frameIndexCapacity = 0;
	FrameIndexQuality frameIndexQuality = INDEX_UNAVAILABLE;
	mutable Mutex indexMutex;
	Task indexTask;
	String filePath;
	uint64_t durationSourceFrames = 0;
	uint64_t indexedSourceFrameEnd = 0;
	mutable portMUX_TYPE timingMux = portMUX_INITIALIZER_UNLOCKED;
	uint64_t elapsedSourceFrames = 0;
	uint64_t decodedSourceFrame = 0;
	uint64_t seekTargetSourceFrame = 0;
	uint64_t elapsedFrameRemainder = 0;
	uint32_t sourceSampleRate = 0;
	uint8_t sourceChannels = 0;
	uint8_t adtsChannelConfiguration = 0;
	uint32_t firstFrameOffset = 0;
	bool readEof = false;
	bool discardPendingRead = false;
	bool rewindAttempted = false;
	ADTSTiming::EofNotification eofNotification;

	bool repeat = false;
	void (*songDoneCallback)() = nullptr;
};


#endif