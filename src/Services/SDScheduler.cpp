#include <SPI.h>
#include "SDScheduler.h"

SDScheduler Sched;

SDScheduler::SDScheduler() : jobs(xQueueCreate(jobCapacity, sizeof(SDJob*))){
}

SDScheduler::~SDScheduler(){
	vQueueDelete(jobs);
}

bool SDScheduler::addJob(SDJob *job){
	if(job == nullptr) return false;
	if(xQueueSend(jobs, &job, 0) == pdTRUE) return true;
	delete job;
	return false;
}

void SDScheduler::loop(uint micros) {
	if (uxQueueMessagesWaiting(jobs) == 0) {
		return;
	}

	SDJob* request = nullptr;

	while(uxQueueMessagesWaiting(jobs) > 0){
		if(xQueueReceive(jobs, &request, 0) != pdTRUE){
			Serial.println("Receive error");
			return;
		}

		if(request != nullptr){
			doJob(request);
		}

		delete request;
	}
}

void SDScheduler::doJob(SDJob* job){
	SPI.setFrequency(60000000);

	if(job->type == SDJob::SD_SEEK){
		bool success = job->file.seek(job->size);

		if(job->result != nullptr){
			SDResult* result = new SDResult();
			result->size = job->size * success;
			result->buffer = job->buffer;
			result->error = success ? 0 : 1;

			*job->result = result;
		}
		return;
	}

	size_t size = job->type == SDJob::SD_READ
				  ? job->file.read(job->buffer, job->size)
				  : job->file.write(job->buffer, job->size);

	if(job->result != nullptr){
		SDResult* result = new SDResult();

		result->error = job->type == SDJob::SD_WRITE && size != job->size;
		result->buffer = job->buffer;
		result->size = size;

		*job->result = result;
	}
}
