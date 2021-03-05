#include "OutputI2S.h"
#include "../AudioSetup.hpp"

OutputI2S::OutputI2S(i2s_config_t config, i2s_pin_config_t pins, int _port) :
		Output::Output(), config(config), pins(pins), port(port){
}

OutputI2S::~OutputI2S(){
}

void OutputI2S::output(size_t numSamples){
	size_t bytesWritten;
	i2s_write(I2S_NUM_0, inBuffer, numSamples*NUM_CHANNELS*BYTES_PER_SAMPLE, &bytesWritten, portMAX_DELAY);
}

void OutputI2S::start(){
	esp_err_t err = i2s_driver_install(I2S_NUM_0, &config, 0, NULL);
	if (err != ESP_OK) {
		Serial.printf("Failed installing I2S driver: %d\n", err);
	}
	err = i2s_set_pin(I2S_NUM_0, &pins);
	if (err != ESP_OK) {
		Serial.printf("Failed setting I2S pin: %d\n", err);
	}
}

void OutputI2S::stop(){
	esp_err_t err = i2s_driver_uninstall(I2S_NUM_0);
	if (err != ESP_OK) {
		Serial.printf("Failed uninstalling I2S driver: %d\n", err);
	}
}