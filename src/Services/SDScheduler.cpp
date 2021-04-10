#include <SPI.h>
#include "SDScheduler.h"
SDScheduler Sched;

SDScheduler::SDScheduler() :jobs(4, sizeof(SDJob*)){

}

void SDScheduler::addJob(SDJob *job){
	jobs.send(job);
}

void SDScheduler::loop(uint micros) {
	if (jobs.count() == 0) {
		return;
	}
	SDJob *request = nullptr;
	while (jobs.count() > 0) {
		jobs.receive(&request);
		doJob(request);
		delete request;
	}
}

void SDScheduler::doJob(SDJob* job){
	SPI.setFrequency(60000000);

	size_t size = job->type == SDJob::SD_READ
			? job->file.read(job->buffer, job->size)
			: job->file.write(job->buffer, job->size);

	SDResult* result = new SDResult();
	result->error = 0;
	result->buffer = job->buffer;
	result->size = size;

	*job->result = result;
}

