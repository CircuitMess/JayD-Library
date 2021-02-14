#include <Arduino.h>
#include <CircuitOS.h>
#include "src/JayD.h"
#include <Loop/LoopManager.h>

#include <SPI.h>
#include <SerialFlash.h>
#include <Devices/SerialFlash/SerialFlashFileAdapter.h>
#include <SD.h>

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

// The pin config as per the setup
const i2s_pin_config_t ringo_pin_config = {
	.bck_io_num = 13,   // BCKL
	.ws_io_num = 15,    // LRCL
	.data_out_num = 22,	//speaker pin
	.data_in_num = 33	//mic pin
};

AudioOutput* output;
void setup(){
	Serial.begin(115200);

	//RINGO TESTING
	//-------------------------------------------
	pinMode(32, OUTPUT);
	digitalWrite(32, LOW);
	pinMode(26, OUTPUT);
	digitalWrite(26, 0);
	pinMode(36, INPUT_PULLUP);
	pinMode(34, INPUT_PULLUP);
	pinMode(33, INPUT_PULLUP);
	pinMode(39, INPUT_PULLUP);
	pinMode(25, OUTPUT);
	digitalWrite(25, 0);
	while (!SD.begin(5, SPI, 8000000)){
		Serial.println("SD ERROR");
	}



	//SPENCER TESTING
	//-------------------------------------------
	// SPIClass spi(3);
	// spi.begin(18, 19, 23, 5);
	// if(!SerialFlash.begin(spi, 5)){
	// 	Serial.println("Flash fail");
	// 	return;
	// }
	

	output = new AudioOutputI2S(i2s_config, ringo_pin_config, 0);
	AudioMixer* mixer = new AudioMixer();
	mixer->addSource(new AudioGeneratorWAV(new File(SD.open("/song1.wav", "r"))));
	// mixer->addSource(new AudioGeneratorWAV(new File(SD.open("/song2.wav", "r"))));
	// mixer->setMixRatio(255);
	output->setSource(mixer);


	// mixer->addSource(new AudioGeneratorWAV(new fs::File(fs::FileImplPtr(new SerialFlashFileAdapter("song2.wav")))));
	// mixer->addSource(new AudioGeneratorWAV(new fs::File(fs::FileImplPtr(new SerialFlashFileAdapter("song1.wav")))));


	// output->setSource(new AudioGeneratorWAV(new fs::File(fs::FileImplPtr(new SerialFlashFileAdapter("joke1.wav")))));
	LoopManager::addListener(output);
	output->setGain(0.1);
}

void loop(){
	LoopManager::loop();
}