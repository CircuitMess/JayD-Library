#ifndef JAYD_LIBRARY_SETTINGS_H
#define JAYD_LIBRARY_SETTINGS_H

#include <Arduino.h>

struct SettingsData {
	uint8_t brightnessLevel = 150; //medium brightness
	uint8_t volumeLevel = 100; //medium volume
};

class SettingsImpl {
public:
	bool begin();

	void store();

	SettingsData &get();

	/**
	 * Resets the data (to zeroes). Doesn't store.
	 */
	void reset();

	uint getVersion(){
		return 1;
	}

	bool isInputTested() const{
		return inputTested;
	}

	void setInputTested(bool inputTested){
		SettingsImpl::inputTested = inputTested;
	}

private:
	SettingsData data;
	bool inputTested = false;
};

extern SettingsImpl Settings;


#endif //JAYD_LIBRARY_SETTINGS_H
