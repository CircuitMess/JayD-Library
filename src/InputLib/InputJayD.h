#ifndef JAYD_INPUTJAYD_H
#define JAYD_INPUTJAYD_H


#include <Loop/LoopListener.h>
#include <Util/Vector.h>
#include <sys/param.h>

#define DEBOUNCE_COUNT 1
#define deviceAddr 0x43
#define getEvents 0x12
#define sendEvents 0x13
#define btnNum 9
#define encNum 7
#define potNum 3
#define resetPin 13


enum DeviceType {
	BTN, ENC, POT
};

struct Event {
	DeviceType deviceType;
	uint8_t deviceID;
	int8_t value;
};

class InputJayD : public LoopListener {

public:

	InputJayD();

	virtual void setBtnPressCallback(uint8_t id, void(*callback)());

	virtual void setBtnReleaseCallback(uint8_t id, void (*callback)());

	virtual void removeBtnPressCallback(uint8_t id);

	virtual void removeBtnReleaseCallback(uint8_t id);

	virtual void setBtnHeldCallback(uint8_t id, uint32_t holdTime, void (*callback)());

	virtual void removeBtnHeldCallback(uint8_t id);

	virtual void setEncoderMovedCallback(uint8_t id, void (*callback)(int8_t value));

	virtual void removeEncoderMovedCallback(uint8_t id);

	virtual void setPotMovedCallback(uint8_t id, void (*callback)(uint8_t value));

	virtual void removePotMovedCallback(uint8_t id);

	static InputJayD *getInstance();

	void loop(uint _time) override;

	uint8_t getNumEvents();

	void fetchEvents(int numEvents);

	void handleButtonEvent(uint8_t id, uint8_t value);

	void handleEncoderEvent(uint8_t id, int8_t value);

	void handlePotentiometerEvent(uint8_t id, uint8_t value);

	void buttonHoldCheck();


protected:

	std::vector<void (*)()> btnPressCallbacks;
	std::vector<void (*)()> btnReleaseCallbacks;
	std::vector<void (*)()> btnHoldCallbacks;
	//std::vector<void (*)(uint)> btnHoldRepeatCallbacks;
	std::vector<void (*)(int8_t)> encMovedCallbacks;
	std::vector<void (*)(uint8_t)> potMovedCallbacks;

	std::vector <uint32_t> btnHoldValue;
	std::vector <uint32_t> btnHoldStart;
	std::vector<bool> wasPressed;

	static InputJayD *instance;

	void reset();

	bool begin();

	bool identify();
};


#endif //JAYD_LIBRARY_INPUTJAYD_H
