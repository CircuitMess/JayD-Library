#ifndef JAYD_H
#define JAYD_H

#include <Arduino.h>
#include <CircuitOS.h>
#include <Loop/LoopManager.h>
#include <Display/Display.h>
#include <Devices/Matrix/Matrix.h>
#include <Devices/Matrix/MatrixAnimGIF.h>
#include "Matrix/MatrixManager.h"
#include <driver/i2s.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <SPI.h>
#include "Settings.h"
#include "Services/SDScheduler.h"
#include "Input/InputJayD.h"
#include "AudioLib/Systems/MixSystem.h"
#include "AudioLib/Systems/PlaybackSystem.h"
#include <Devices/Matrix/IS31FL3731.h>
#include "JayDDisplay.h"

extern const i2s_pin_config_t i2s_pin_config;
extern Matrix LEDmatrix;
extern MatrixManager matrixManager;
extern IS31FL3731 charlie;

class JayDImpl {
public:
	JayDImpl();
	void initVer(int override = -1); // Initializes version and pins; also called from begin()

	void begin();

	Display& getDisplay();

	File SD_open(const char* path, const char* mode = FILE_READ);
	File SD_open(String path, const char* mode = FILE_READ);

	bool SD_exists(const char* path);
	bool SD_exists(const String& path);
	bool SD_remove(const char* path);
	bool SD_remove(const String& path);

	bool SD_begin();
	bool SD_begin(uint8_t ssPin, SPIClass &spi);
	bool SD_begin(const char* mountpoint, bool mode1bit);
	void SD_end();

private:
	Display display;

	enum class Ver { v1_0, v1_1, v1_2, v1_3 } ver = Ver::v1_0;
	bool verInited = false;

};

extern JayDImpl JayD;

#endif