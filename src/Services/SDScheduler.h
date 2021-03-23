#ifndef JAYD_LIBRARY_SDSCHEDULER_H
#define JAYD_LIBRARY_SDSCHEDULER_H

#include <FS.h>
#include <vector>
#include <Loop/LoopListener.h>
#include <queue>

struct SDResult {
	uint8_t error;
	size_t size;
	uint8_t* buffer;
};

struct SDJob {
	enum { SD_WRITE, SD_READ } type;
	fs::File file;
	size_t size;
	uint8_t* buffer;
	SDResult** result;
};

class SDScheduler : public LoopListener {
public:
	void addJob(const SDJob& job);
	void loop(uint micros) override;
private:
	std::queue<SDJob> jobs;

	void doJob(SDJob& job);
};

extern SDScheduler Sched;

#endif //JAYD_LIBRARY_SDSCHEDULER_H
