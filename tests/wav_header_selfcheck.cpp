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
	assert(applied.find("out->addOutput") == std::string::npos);
	assert(applied.find("while(") == std::string::npos);
	assert(applied.find("Sched.loop") == std::string::npos);
	assert(applied.find("delayMicroseconds") == std::string::npos);

	std::ifstream wavFile("src/AudioLib/OutputWAV.cpp");
	assert(wavFile.good());
	const std::string wavSource(
			(std::istreambuf_iterator<char>(wavFile)),
			std::istreambuf_iterator<char>()
	);
	const size_t beginBegin = wavSource.find("bool OutputWAV::begin(");
	const size_t beginEnd = wavSource.find("void OutputWAV::finish()", beginBegin);
	assert(beginBegin != std::string::npos);
	assert(beginEnd != std::string::npos);
	const std::string begin = wavSource.substr(beginBegin, beginEnd - beginBegin);
	assert(begin.find("while(") == std::string::npos);
	assert(begin.find("Sched.loop") == std::string::npos);
	const size_t wavServiceBegin = wavSource.find("void OutputWAV::service()");
	const size_t wavServiceEnd = wavSource.find("bool OutputWAV::isFinalized()", wavServiceBegin);
	assert(wavServiceBegin != std::string::npos);
	assert(wavServiceEnd != std::string::npos);
	const std::string wavService =
			wavSource.substr(wavServiceBegin, wavServiceEnd - wavServiceBegin);
	assert(wavService.find("while(") == std::string::npos);
	assert(wavService.find("Sched.loop") == std::string::npos);
	assert(wavSource.find("FinalizeStage::INIT_SEEK_QUEUE") != std::string::npos);
	assert(wavSource.find("FinalizeStage::INIT_HEADER_QUEUE") != std::string::npos);
	assert(wavSource.find("if(finalizeResult == nullptr) return;") != std::string::npos);

	const size_t serviceBegin = source.find("void MixSystem::serviceRecording()");
	const size_t serviceEnd = source.find("void MixSystem::finishRecordingSync()", serviceBegin);
	assert(serviceBegin != std::string::npos);
	assert(serviceEnd != std::string::npos);
	const std::string service = source.substr(serviceBegin, serviceEnd - serviceBegin);
	assert(service.find("fsOut->isReady()") != std::string::npos);
	assert(service.find("recordingMutex.lock()") != std::string::npos);
	assert(service.find("out->addOutput") != std::string::npos);

	const size_t stopBegin = source.find("bool MixSystem::stopRecording()");
	const size_t stopEnd = source.find("void MixSystem::_startRecording()", stopBegin);
	assert(stopBegin != std::string::npos);
	assert(stopEnd != std::string::npos);
	const std::string stop = source.substr(stopBegin, stopEnd - stopBegin);
	assert(stop.find("recordingMutex.lock()") != std::string::npos);
	assert(stop.find("recordingState = previousState") == std::string::npos);

	std::ifstream schedulerFile("src/Services/SDScheduler.cpp");
	assert(schedulerFile.good());
	const std::string scheduler(
			(std::istreambuf_iterator<char>(schedulerFile)),
			std::istreambuf_iterator<char>()
	);
	const size_t tryAddBegin = scheduler.find("bool SDScheduler::tryAddJob(");
	const size_t tryAddEnd = scheduler.find("void SDScheduler::loop(", tryAddBegin);
	assert(tryAddBegin != std::string::npos);
	assert(tryAddEnd != std::string::npos);
	const std::string tryAdd = scheduler.substr(tryAddBegin, tryAddEnd - tryAddBegin);
	assert(tryAdd.find("xQueueSend(jobs, &job, 0)") != std::string::npos);
	assert(tryAdd.find("portMAX_DELAY") == std::string::npos);
	assert(wavSource.find("Sched.tryAddJob(") != std::string::npos);
	assert(wavSource.find("Sched.addJob(") == std::string::npos);
	return 0;
}
