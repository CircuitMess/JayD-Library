#ifndef JAYD_LIBRARY_SDSCHEDULER_H
#define JAYD_LIBRARY_SDSCHEDULER_H

#include <FS.h>
#include <Loop/LoopListener.h>
#include <freertos/queue.h>

struct SDResult {
	uint8_t error;
	size_t size;
	uint8_t* buffer;
};

struct SDJob {
	enum Type { SD_WRITE, SD_READ, SD_SEEK } type;
	fs::File file;
	size_t size;
	uint8_t* buffer;
	SDResult** result;
};

class SDScheduler : public LoopListener {
public:
	SDScheduler();
	~SDScheduler();

	void addJob(SDJob *job);
	bool tryAddJob(SDJob *job);
	void loop(uint micros) override;
private:
	static constexpr uint8_t jobCapacity = 8;
	QueueHandle_t jobs;

	void doJob(SDJob* job);

};

extern SDScheduler Sched;

#endif //JAYD_LIBRARY_SDSCHEDULER_H
