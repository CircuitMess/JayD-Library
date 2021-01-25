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

enum DeviceType{BTN,ENC,POT};

struct Event{
	DeviceType deviceType;
	uint8_t deviceID;
	int8_t value;
};

class InputJayD : public LoopListener {

public:

	InputJayD();

	virtual void setBtnPressCallback(uint8_t _id, void(*callback)());

	virtual void setBtnReleaseCallback(uint8_t _id, void (*callback)());

	virtual void removeBtnPressCallback(uint8_t _id);

	virtual void removeBtnReleaseCallback(uint8_t _id);

	virtual void setButtonHeldCallback(uint8_t _id, uint32_t holdTime, void (*callback)());

	//virtual void setButtonHeldRepeatCallback(uint8_t _id, uint32_t periodTime, void (*callback)(uint));

	//virtual uint32_t getButtonMillis(uint8_t _id);

	//virtual void setAnyKeyCallback(void (*callback)(), bool returnAfterCallback = false);

	//virtual void registerButton(uint8_t _id);
	//virtual void preregisterButtons(Vector<uint8_t> pins);

	virtual void setEncoderMovedCallback(uint8_t _id, void (*callback)(int8_t value));

	virtual void removeEncoderMovedCallback(uint8_t _id);

	virtual void setPotMovedCallback(uint8_t _id, void (*callback)(uint8_t value));

	virtual void removePotMovedCallback(uint8_t _id);

	static InputJayD *getInstance();

	void loop(uint _time) override;

	uint8_t getNumEvents();

	void fetchEvents(int numEvents);

	void handleButtonEvent(uint8_t _id,uint8_t _value);
	void handleEncoderEvent(uint8_t _id,uint8_t _value);
	void handlePotentiometerEvent(uint8_t _id,uint8_t _value);


	void separateDevice(Event &event);

protected:

	std::vector<void (*)()> btnPressCallbacks;
	std::vector<void (*)()> btnReleaseCallbacks;
	std::vector<void (*)()> btnHoldCallbacks;
	std::vector<void (*)(uint)> btnHoldRepeatCallbacks;
	std::vector<void (*)(int8_t)> encMovedCallbacks;
	std::vector<void (*)(uint8_t)> potMovedCallbacks;

	std::vector<uint32_t> btnHoldValue;
	std::vector<uint32_t> btnHoldStart;
	std::vector<uint8_t> btnState;

	static InputJayD *instance;

	uint8_t deviceId;
	int8_t valueData;
	uint8_t device;
	uint8_t id;
};


#endif //JAYD_LIBRARY_INPUTJAYD_H
