#include <Arduino.h>
#include <CircuitOS.h>
#include "src/JayD.h"
#include <Loop/LoopManager.h>

#include <SPI.h>
#include <SerialFlash.h>
#include <Devices/SerialFlash/SerialFlashFileAdapter.h>

const i2s_config_t i2s_config = {
	.mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX), // Receive and transfer
	.sample_rate = 16000,                         // 44.1KHz
	.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // could only get it to work with 32bits
	.channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT, // although the SEL config should be left, it seems to transmit on right
	.communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
	.intr_alloc_flags = 0,     // Interrupt level 1
	.dma_buf_count = 16,                           // number of buffers
	.dma_buf_len = 60,                              // 60 samples per buffer (8 minimum)
	.use_apll = false
};

// The pin config as per the setup
const i2s_pin_config_t spencer_pin_config = {
	.bck_io_num = 16,   // BCKL
	.ws_io_num = 27,    // LRCL
	.data_out_num = 4,	//speaker pin
	.data_in_num = 32	//mic pin
};

AudioOutput* output;
void setup(){
	Serial.begin(115200);


	SPIClass spi(3);
	spi.begin(18, 19, 23, 5);
	if(!SerialFlash.begin(spi, 5)){
		Serial.println("Flash fail");
		return;
	}

	output = new AudioOutputI2S(i2s_config, spencer_pin_config, 0);
	output->setSource(new AudioGeneratorWAV(new fs::File(fs::FileImplPtr(new SerialFlashFileAdapter("joke.wav")))));

	output->start();
	output->setGain(0.05);
}

void loop(){
	LoopManager::loop();
}