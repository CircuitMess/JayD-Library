#ifndef BYTEBOI_LIBRARY_BYTEBOIDISPLAY_H
#define BYTEBOI_LIBRARY_BYTEBOIDISPLAY_H

#include <Arduino.h>

#define LGFX_USE_V1
#define ARDUINO_LOLIN_D32_PRO

#include <LovyanGFX.hpp>

class JayDDisplay : public lgfx::LGFX_Device {
public:

	static lgfx::Panel_ST7735S* panel1();
	static lgfx::Panel_ST7735S* panel2();
	static lgfx::Panel_ST7735S* panel3();

};


#endif //BYTEBOI_LIBRARY_BYTEBOIDISPLAY_H
