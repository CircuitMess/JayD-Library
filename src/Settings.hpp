#ifndef JAYD_LIBRARY_SETTINGS_HPP
#define JAYD_LIBRARY_SETTINGS_HPP

#include <Arduino.h>

struct SettingsData {
	uint8_t brightnessLevel = 150; //medium brightness
	uint8_t volumeLevel = 100; //medium volume
};

class SettingsImpl {
public:
	bool begin();

	void store();

	SettingsData& get();

	/**
	 * Resets the data (to zeroes). Doesn't store.
	 */
	void reset();

	uint getVersion(){
		return 1;
	}

private:
	SettingsData data;

};

extern SettingsImpl Settings;


#endif //JAYD_LIBRARY_SETTINGS_HPP
