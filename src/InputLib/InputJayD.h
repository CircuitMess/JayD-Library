#ifndef JAYD_INPUTJAYD_H
#define JAYD_INPUTJAYD_H


#include <Loop/LoopListener.h>
#include <Util/Vector.h>
#include <sys/param.h>

#define DEBOUNCE_COUNT 1
#define deviceAddr 0x43
#define getEvents 0x12
#define sendEvents 0x13


class InputJayD : public LoopListener {

public:

	InputJayD();

	virtual void setBtnPressCallback(uint8_t _id, void(*callback)());

	virtual void setBtnReleaseCallback(uint8_t _id, void (*callback)());

	virtual void removeBtnPressCallback(uint8_t _id);

	virtual void removeBtnReleaseCallback(uint8_t _id);

	virtual void setButtonHeldCallback(uint8_t _id, uint32_t holdTime, void (*callback)());

	virtual void setButtonHeldRepeatCallback(uint8_t _id, uint32_t periodTime, void (*callback)(uint));

	virtual uint32_t getButtonMillis(uint8_t _id);

//	virtual void setAnyKeyCallback(void (*callback)(), bool returnAfterCallback = false);

	//virtual void registerButton(uint8_t _id);
//	virtual void preregisterButtons(Vector<uint8_t> pins);

	virtual void setEncoderMovedCallback(uint8_t _id, void (*callback)(int8_t value));

	virtual void removeEncoderMovedCallback(uint8_t _id);

	virtual void setPotMovedCallback(uint8_t _id, void (*callback)(uint8_t value));

	virtual void removePotMovedCallback(uint8_t _id);

	static InputJayD *getInstance();

	void loop(uint _time) override;

protected:
	std::vector<void (*)()> btnPressCallback;
	std::vector<void (*)()> btnReleaseCallback;
	std::vector<void (*)()> btnHoldCallback;
	std::vector<void (*)(uint)> btnHoldRepeatCallback;
	std::vector<void (*)(int8_t)> encMovedCallback;
	std::vector<void (*)(uint8_t)> potMovedCallback;


	std::vector<uint32_t> btnHoldValue;
	std::vector<uint32_t> btnHoldRepeatValue;
	std::vector<uint32_t> btnHoldStart;
	std::vector<uint8_t> buttons;
	std::vector<uint8_t> btnCount;
	std::vector<uint8_t> btnState;
	std::vector<bool> btnHoldOver;
	std::vector<uint32_t> btnHoldRepeatCounter;
	std::vector<int32_t> encoderValue;

	void (*anyKeyCallback)(void);

	bool anyKeyCallbackReturn;


	static InputJayD *instance;

	//virtual void scanButtons() = 0;
	//void registerButton(uint8_t pin);


	uint8_t numEventsAddr;
	uint8_t numEventsData;
	uint8_t outputAddr;
	int8_t valueData;
	uint8_t device;
	uint8_t deviceId;
	uint8_t id;
	uint8_t temp=0;
};



#endif //JAYD_LIBRARY_INPUTJAYD_H
