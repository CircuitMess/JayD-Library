#ifndef JAYD_OUTPUT_H
#define JAYD_OUTPUT_H

#include <Arduino.h>
#include <Loop/LoopListener.h>
class Generator;

class Output : public LoopListener
{
public:
	Output(bool timed = false);
	~Output();
	void setSource(Generator* gen);
	void loop(uint _time) override;
	virtual void stop() = 0;
	virtual void start() = 0;
	void setGain(float gain);
	float getGain();
	bool isRunning();

protected:
	virtual void output(size_t numSamples) = 0;
	int16_t *inBuffer = nullptr;

private:
	float gain = 1.0; //0 - 1.0
	bool running = false;
	uint32_t lastSample = 0;
	size_t receivedSamples = 0;
	bool timed = false;
};


#endif