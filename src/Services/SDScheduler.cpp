#include <SPI.h>
#include "SDScheduler.h"
SDScheduler Sched;

SDScheduler::SDScheduler() :jobs(4, sizeof(SDJob*)){

}

void SDScheduler::addJob(SDJob *job){
	Serial.println("Added job: ");
	Serial.println((size_t)job);
	jobs.send(job);
}

void SDScheduler::loop(uint micros) {
	if (jobs.count() == 0) {
		return;
	}
	SDJob *request = nullptr;
	Serial.print("number of jobs: ");
	Serial.println(jobs.count());
	while (jobs.count() > 0) {
		jobs.receive(&request);
		if(request == nullptr){
			Serial.println("NULLLPTR");
			return;
		}else{
			Serial.println("doing job:");
			Serial.println((size_t)request);
			Serial.flush();
		}
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

