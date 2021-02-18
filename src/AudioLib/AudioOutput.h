#ifndef JAYD_AUDIOOUTPUT_H
#define JAYD_AUDIOOUTPUT_H

#include <Arduino.h>
#include <Loop/LoopListener.h>
class AudioGenerator;

class AudioOutput : public LoopListener
{
public:
	AudioOutput();
	~AudioOutput();
	void setSource(AudioGenerator* gen);
	void loop(uint _time) override;

	void setGain(float gain);
	float getGain();
	bool isRunning();

protected:
	virtual void output(size_t numBytes) = 0;
	virtual void stop() = 0;
	virtual void start() = 0;
	int16_t *inBuffer;

private:
	float gain; //0 - 1.0
	AudioGenerator* generator;
	bool running;
};


#endif