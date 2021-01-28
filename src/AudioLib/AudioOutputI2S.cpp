#include "AudioOutputI2S.h"

AudioOutputI2S::AudioOutputI2S(i2s_config_t config, i2s_pin_config_t pins, int _port) :
		AudioOutput::AudioOutput(), config(config), pins(pins), port(port){
	i2sBegin();
}

AudioOutputI2S::~AudioOutputI2S(){
}

void AudioOutputI2S::output(){
	size_t bytesWritten;
	i2s_write(I2S_NUM_0, inBuffer, 800*sizeof(int16_t), &bytesWritten, portMAX_DELAY);
}

void AudioOutputI2S::i2sBegin(){
	// The I2S config as per the example

	// Configuring the I2S driver and pins.
	// This function must be called before any I2S driver read/write operations.
	esp_err_t err = i2s_driver_install(I2S_NUM_0, &config, 0, NULL);
	if (err != ESP_OK) {
		Serial.printf("Failed installing I2S driver: %d\n", err);
		while (true);
	}
	err = i2s_set_pin(I2S_NUM_0, &pins);
	if (err != ESP_OK) {
		Serial.printf("Failed setting I2S pin: %d\n", err);
		while (true);
	}
	Serial.println("I2S driver installed.");
}