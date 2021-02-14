#ifndef JAYD_AUDIOOUTPUTI2S_H
#define JAYD_AUDIOOUTPUTI2S_H

#include "AudioOutput.h"
#include <driver/i2s.h>
class AudioOutputI2S : public AudioOutput
{
public:
	AudioOutputI2S(i2s_config_t _config, i2s_pin_config_t _pins, int port);
	~AudioOutputI2S();

protected:
	void output() override;
	void start() override;
	void stop() override;

private:
	i2s_config_t config;
	i2s_pin_config_t pins;
	int port;
};


#endif