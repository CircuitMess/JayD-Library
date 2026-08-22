#include "OutputWAV.h"
#include "../AudioSetup.hpp"
#include "../PerfMon.h"

OutputWAV::OutputWAV(){
	freeBuffers.reserve(OUTWAV_BUFCOUNT);
	for(int i = 0; i < OUTWAV_BUFCOUNT; i++){
		outBuffers[i] = new DataBuffer(OUTWAV_BUFSIZE);
		freeBuffers.push_back(i);
	}
}

OutputWAV::OutputWAV(const fs::File& file) : OutputWAV(){
	this->file = file;
}

OutputWAV::~OutputWAV(){
	delete finalizeResult;
	for(auto result : writeResult){
		delete result;
	}
	for(auto& outBuffer : outBuffers){
		delete outBuffer;
	}
}

const fs::File& OutputWAV::getFile() const{
	return file;
}

void OutputWAV::setFile(const fs::File& file){
	OutputWAV::file = file;
}

void OutputWAV::output(size_t numSamples){
	Profiler.start("WAV write process");
	service();
	Profiler.end();

	const size_t size = numSamples * NUM_CHANNELS * BYTES_PER_SAMPLE;
	if(error != RecordingError::NONE || finalizeStage != FinalizeStage::ACTIVE){
		droppedBytes += size;
		return;
	}

	if(freeBuffers.empty()){
		droppedBytes += size;
		fail(RecordingError::BUFFER_OVERRUN);
		return;
	}

	DataBuffer* buffer = outBuffers[freeBuffers.front()];
	memcpy(buffer->writeData(), this->inBuffer, size);
	buffer->writeMove(size);

	if(buffer->readAvailable() >= OUTWAV_WRITESIZE){
		Profiler.start("WAV write add");
		if(!addWriteJob()){
			droppedBytes += buffer->readAvailable();
			buffer->clear();
			fail(RecordingError::QUEUE_FULL);
		}
		Profiler.end();
	}
}

void OutputWAV::init(){
	if(!prepared && file) begin(file);
	if(!prepared || !file || finalizeStage != FinalizeStage::ACTIVE){
		Serial.println("Output file not open");
		fail(RecordingError::OPEN_FAILED);
	}
}

void OutputWAV::deinit(){
	finish();
}

bool OutputWAV::begin(const fs::File& outputFile){
	if(finalizeStage != FinalizeStage::DONE || hasPendingWrites()) return false;

	file = outputFile;
	bytesWritten = 0;
	droppedBytes = 0;
	error = RecordingError::NONE;
	prepared = false;
	freeBuffers.clear();
	for(uint8_t i = 0; i < OUTWAV_BUFCOUNT; i++){
		outBuffers[i]->clear();
		writePending[i] = false;
		writeSize[i] = 0;
		freeBuffers.push_back(i);
	}

	if(!file){
		fail(RecordingError::OPEN_FAILED);
		return false;
	}

	header = makeWavHeader(0, NUM_CHANNELS, SAMPLE_RATE, BYTES_PER_SAMPLE);
	if(!writeInitialHeader()) return false;
	prepared = true;
	finalizeStage = FinalizeStage::ACTIVE;
	return true;
}

void OutputWAV::finish(){
	if(finalizeStage != FinalizeStage::ACTIVE || !prepared) return;

	if(!freeBuffers.empty()){
		DataBuffer* buffer = outBuffers[freeBuffers.front()];
		if(buffer->readAvailable() > 0){
			if(error != RecordingError::NONE){
				droppedBytes += buffer->readAvailable();
				buffer->clear();
			}else if(!addWriteJob()){
				droppedBytes += buffer->readAvailable();
				buffer->clear();
				fail(RecordingError::QUEUE_FULL);
			}
		}
	}
	finalizeStage = FinalizeStage::DRAIN;
}

void OutputWAV::service(){
	processWriteJob();
	if(finalizeStage == FinalizeStage::DONE ||
	   finalizeStage == FinalizeStage::ACTIVE ||
	   (finalizeStage == FinalizeStage::DRAIN && hasPendingWrites())) return;

	if(finalizeStage == FinalizeStage::DRAIN){
		header = makeWavHeader(bytesWritten, NUM_CHANNELS, SAMPLE_RATE, BYTES_PER_SAMPLE);
		if(!queueFinalizeJob(SDJob::SD_SEEK)){
			fail(RecordingError::FINALIZE_FAILED);
			finalizeStage = FinalizeStage::DONE;
		}else{
			finalizeStage = FinalizeStage::SEEK;
		}
		return;
	}

	if(finalizeResult == nullptr) return;
	const bool success = finalizeResult->error == 0 &&
			(finalizeStage == FinalizeStage::SEEK || finalizeResult->size == sizeof(WavHeader));
	delete finalizeResult;
	finalizeResult = nullptr;
	if(!success){
		fail(RecordingError::FINALIZE_FAILED);
		finalizeStage = FinalizeStage::DONE;
		return;
	}

	if(finalizeStage == FinalizeStage::SEEK){
		if(!queueFinalizeJob(SDJob::SD_WRITE)){
			fail(RecordingError::FINALIZE_FAILED);
			finalizeStage = FinalizeStage::DONE;
		}else{
			finalizeStage = FinalizeStage::HEADER;
		}
	}else{
		finalizeStage = FinalizeStage::DONE;
		prepared = false;
	}
}

bool OutputWAV::isFinalized() const{
	return finalizeStage == FinalizeStage::DONE;
}

RecordingError OutputWAV::getError() const{
	return error;
}

uint32_t OutputWAV::getBytesWritten() const{
	return bytesWritten;
}

uint32_t OutputWAV::getDroppedBytes() const{
	return droppedBytes;
}

uint32_t OutputWAV::getDurationMs() const{
	const uint32_t byteRate = SAMPLE_RATE * NUM_CHANNELS * BYTES_PER_SAMPLE;
	return byteRate == 0 ? 0 : static_cast<uint64_t>(bytesWritten) * 1000 / byteRate;
}

bool OutputWAV::addWriteJob(){
	if(freeBuffers.empty()) return false;
	const uint8_t i = freeBuffers.front();
	const size_t size = outBuffers[i]->readAvailable();
	if(size == 0) return true;

	if(!Sched.addJob(new SDJob{
			 .type = SDJob::SD_WRITE,
			 .file = file,
			 .size = size,
			 .buffer = const_cast<uint8_t*>(outBuffers[i]->readData()),
			 .result = &writeResult[i]
	 })) return false;

	freeBuffers.erase(freeBuffers.begin());
	writePending[i] = true;
	writeSize[i] = size;
	return true;
}

void OutputWAV::processWriteJob(){
	for(int i = 0; i < OUTWAV_BUFCOUNT; i++){
		if(!writePending[i]) continue;
		if(writeResult[i] == nullptr) continue;

		const size_t actual = writeResult[i]->size > writeSize[i]
				? writeSize[i]
				: writeResult[i]->size;
		bytesWritten += actual;
		if(writeResult[i]->error != 0 || actual != writeSize[i]){
			droppedBytes += writeSize[i] - actual;
			fail(RecordingError::WRITE_FAILED);
		}

		outBuffers[i]->clear();
		delete writeResult[i];
		writeResult[i] = nullptr;
		writePending[i] = false;
		writeSize[i] = 0;
		freeBuffers.push_back(i);
	}
}

bool OutputWAV::hasPendingWrites() const{
	for(bool pending : writePending){
		if(pending) return true;
	}
	return false;
}

bool OutputWAV::writeInitialHeader(){
	if(!queueFinalizeJob(SDJob::SD_SEEK)){
		fail(RecordingError::QUEUE_FULL);
		return false;
	}
	while(finalizeResult == nullptr) Sched.loop(0);
	const bool seekSuccess = finalizeResult->error == 0;
	delete finalizeResult;
	finalizeResult = nullptr;
	if(!seekSuccess){
		fail(RecordingError::WRITE_FAILED);
		return false;
	}

	if(!queueFinalizeJob(SDJob::SD_WRITE)){
		fail(RecordingError::QUEUE_FULL);
		return false;
	}
	while(finalizeResult == nullptr) Sched.loop(0);
	const bool writeSuccess = finalizeResult->error == 0 && finalizeResult->size == sizeof(WavHeader);
	delete finalizeResult;
	finalizeResult = nullptr;
	if(!writeSuccess) fail(RecordingError::WRITE_FAILED);
	return writeSuccess;
}

bool OutputWAV::queueFinalizeJob(SDJob::Type type){
	return Sched.addJob(new SDJob {
			.type = type,
			.file = file,
			.size = type == SDJob::SD_SEEK ? 0 : sizeof(WavHeader),
			.buffer = type == SDJob::SD_SEEK ? nullptr : reinterpret_cast<uint8_t*>(&header),
			.result = &finalizeResult
	});
}

void OutputWAV::fail(RecordingError recordingError){
	if(error == RecordingError::NONE) error = recordingError;
}
