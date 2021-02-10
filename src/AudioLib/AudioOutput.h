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
	void stop();
	void start();
	void setGain(float gain);
	float getGain();
	bool isRunning();

protected:
	virtual void output() = 0;
	int16_t *inBuffer;

private:
	float gain; //0 - 1.0
	AudioGenerator* generator;
	bool running;
	const size_t bufferSize = 800; //800 samples, 16-bit stereo

};


#endif