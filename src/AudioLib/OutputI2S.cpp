#include "OutputI2S.h"

OutputI2S::OutputI2S(i2s_config_t config, i2s_pin_config_t pins, int _port, StreamableHTTPClient *_http) :
		Output::Output(), config(config), pins(pins), port(port), http(_http){
}

OutputI2S::~OutputI2S(){
}

void OutputI2S::output(size_t numBytes){
	size_t bytesWritten;
	http->send((uint8_t*)inBuffer, numBytes);
	i2s_write(I2S_NUM_0, inBuffer, numBytes, &bytesWritten, 0);
}

void OutputI2S::start(){
	// The I2S config as per the example

	// Configuring the I2S driver and pins.
	// This function must be called before any I2S driver read/write operations.
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

	int code = http->finish();
	if(code != 200){
		Serial.printf("HTTP code %d\n", code);
		http->end();
		http->getStream().stop();
		http->getStream().flush();
		return;
	}

	esp_err_t err = i2s_driver_uninstall(I2S_NUM_0);
	if (err != ESP_OK) {
		Serial.printf("Failed uninstalling I2S driver: %d\n", err);
	}
}