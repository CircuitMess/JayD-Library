#ifndef JAYD_OUTPUT_H
#define JAYD_OUTPUT_H

#include <Arduino.h>
#include <Loop/LoopListener.h>
class Generator;

class Output : public LoopListener
{
public:
	Output();
	~Output();
	void setSource(Generator* gen);
	void loop(uint _time) override;
	virtual void stop() = 0;
	virtual void start() = 0;
	void setGain(float gain);
	float getGain();
	bool isRunning();

protected:
	virtual void output(size_t numBytes) = 0;
	int16_t *inBuffer;

private:
	float gain; //0 - 1.0
	Generator* generator;
	bool running;
};


#endif