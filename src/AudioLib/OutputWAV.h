#ifndef JAYD_OUTPUTWAV_H
#define JAYD_OUTPUTWAV_H

#include "Output.h"
#include "../AudioSetup.hpp"
#include <FS.h>
#include <aacenc_lib.h>
#include <Buffer/DataBuffer.h>
#include "../Services/SDScheduler.h"
#include "WavHeader.h"

#define OUTWAV_BUFSIZE 2 * 1024 * NUM_CHANNELS
#define OUTWAV_WRITESIZE 1 * 1024 * NUM_CHANNELS // should be smaller than BUFSIZE
#define OUTWAV_BUFCOUNT 16

enum class RecordingError : uint8_t {
	NONE,
	SD_UNAVAILABLE,
	OPEN_FAILED,
	WRITE_FAILED,
	FINALIZE_FAILED,
	BUFFER_OVERRUN,
	QUEUE_FULL
};

class OutputWAV : public Output
{
public:
	OutputWAV();
	OutputWAV(const fs::File& file);
	~OutputWAV();
	void init() override;
	void deinit() override;
	const fs::File& getFile() const;
	void setFile(const fs::File& file);
	bool begin(const fs::File& file);
	void finish();
	void service();
	bool isFinalized() const;
	RecordingError getError() const;
	uint32_t getBytesWritten() const;
	uint32_t getDroppedBytes() const;
	uint32_t getDurationMs() const;

protected:
	void output(size_t numSamples) override;

private:
	fs::File file;
	uint32_t bytesWritten = 0;
	uint32_t droppedBytes = 0;
	RecordingError error = RecordingError::NONE;
	bool prepared = false;

	bool writePending[OUTWAV_BUFCOUNT] = { false };
	SDResult* writeResult[OUTWAV_BUFCOUNT] = { nullptr };
	size_t writeSize[OUTWAV_BUFCOUNT] = { 0 };
	bool addWriteJob();
	void processWriteJob();
	bool hasPendingWrites() const;

	DataBuffer* outBuffers[OUTWAV_BUFCOUNT] = { nullptr };
	std::vector<uint8_t> freeBuffers;

	enum class FinalizeStage : uint8_t { DONE, ACTIVE, DRAIN, SEEK, HEADER };
	FinalizeStage finalizeStage = FinalizeStage::DONE;
	SDResult* finalizeResult = nullptr;
	WavHeader header = {};

	bool writeInitialHeader();
	bool queueFinalizeJob(SDJob::Type type);
	void fail(RecordingError recordingError);
};


#endif