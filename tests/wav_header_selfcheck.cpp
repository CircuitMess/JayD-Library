#include <assert.h>
#include <fstream>
#include <iterator>
#include <string.h>
#include <string>
#include "../src/AudioLib/RecordingFinalize.h"
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

	uint8_t queueFailures = 0;
	assert(recordFinalizeEnqueue(false, queueFailures, 3) == FinalizeEnqueueResult::RETRY);
	assert(recordFinalizeEnqueue(false, queueFailures, 3) == FinalizeEnqueueResult::RETRY);
	assert(recordFinalizeEnqueue(true, queueFailures, 3) == FinalizeEnqueueResult::QUEUED);
	assert(queueFailures == 2);
	assert(recordFinalizeEnqueue(false, queueFailures, 3) == FinalizeEnqueueResult::EXHAUSTED);

	std::ifstream sourceFile("src/AudioLib/Systems/MixSystem.cpp");
	assert(sourceFile.good());
	const std::string source(
			(std::istreambuf_iterator<char>(sourceFile)),
			std::istreambuf_iterator<char>()
	);
	const size_t startBegin = source.find("bool MixSystem::startRecording()");
	const size_t startEnd = source.find("bool MixSystem::stopRecording()", startBegin);
	assert(startBegin != std::string::npos);
	assert(startEnd != std::string::npos);
	const std::string start = source.substr(startBegin, startEnd - startBegin);
	assert(start.find("enqueueRequest") != std::string::npos);
	assert(start.find("fsOut->") == std::string::npos);
	assert(start.find("SD.") == std::string::npos);
	assert(start.find("fileOut") == std::string::npos);

	const size_t appliedBegin = source.find("void MixSystem::_startRecording()");
	const size_t appliedEnd = source.find("void MixSystem::_stopRecording()", appliedBegin);
	assert(appliedBegin != std::string::npos);
	assert(appliedEnd != std::string::npos);
	const std::string applied = source.substr(appliedBegin, appliedEnd - appliedBegin);
	assert(applied.find("SD.open") != std::string::npos);
	assert(applied.find("fsOut->begin") != std::string::npos);
	assert(applied.find("fsOut->invalidateFile") != std::string::npos);
	assert(applied.find("recordingApplied = true") != std::string::npos);
	return 0;
}
