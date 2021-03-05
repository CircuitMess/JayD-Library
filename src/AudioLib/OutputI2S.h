#ifndef JAYD_OUTPUTI2S_H
#define JAYD_OUTPUTI2S_H

#include "Output.h"
#include <driver/i2s.h>
class OutputI2S : public Output
{
public:
	OutputI2S(i2s_config_t _config, i2s_pin_config_t _pins, int port);
	~OutputI2S();
	void start() override;
	void stop() override;

protected:
	void output(size_t numBytes) override;

private:
	i2s_config_t config;
	i2s_pin_config_t pins;
	int port;
};


#endif