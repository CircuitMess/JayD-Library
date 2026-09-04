#ifndef JAYD_LIBRARY_PINS_H
#define JAYD_LIBRARY_PINS_H

#include <cstdint>
#include <unordered_map>
#include <vector>

#define PIN(x) Pins::get(Pin::x)

enum class Pin : uint8_t {
	PIN_BL,
	SD_CS,
	ENC_MID,
	ENC_L1,
	ENC_L2,
	ENC_L3,
	ENC_R1,
	ENC_R2,
	ENC_R3,
	POT_L,
	POT_MID,
	POT_R,
	BTN_L,
	BTN_R,
	BTN_MID,
	BTN_L1,
	BTN_L2,
	BTN_L3,
	BTN_R1,
	BTN_R2,
	BTN_R3,
	I2S_WS,
	I2S_DO,
	I2S_BCK,
	I2S_DI,
	I2C_SDA,
	I2C_SCL,
	SPI_SCK,
	SPI_MISO,
	SPI_MOSI,
	SPI_SS
};

class Pins {
	struct PinHash {
		std::size_t operator()(Pin pin) const{
			return static_cast<std::size_t>(pin);
		}
	};

	typedef std::unordered_map<Pin, int, PinHash> PinMap;

public:
	static int get(Pin pin);

	static void setLatest();

private:
	Pins();

	PinMap* currentMap = nullptr;

	static Pins* instance;

	void initPinMaps();

	//For original Jay-D, Jay-D v2
	PinMap Revision1;

	//For Jay-D v3 (sd fix)
	PinMap Revision2;

	std::vector<PinMap*> pinMaps = { &Revision1, &Revision2 };
};

extern Pins JayDPins;

#endif //JAYD_LIBRARY_PINS_H
