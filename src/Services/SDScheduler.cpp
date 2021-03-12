#include "SDScheduler.h"

SDScheduler Sched;

void SDScheduler::addJob(const SDJob& job){
	jobs.push(job);
}

void SDScheduler::loop(uint micros){
	if(jobs.empty()) return;
	doJob(jobs.front());
	jobs.pop();
}

void SDScheduler::doJob(SDJob& job){
	size_t size = job.type == SDJob::SD_READ
			? job.file.read(job.buffer, job.size)
			: job.file.write(job.buffer, job.size);

	SDResult* result = new SDResult();
	result->error = 0;
	result->buffer = job.buffer;
	result->size = size;

	*job.result = result;
}
