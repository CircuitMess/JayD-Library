#include "InputJayD.h"
#include <Wire.h>
#include <driver/i2s.h>


InputJayD *InputJayD::instance;

InputJayD::InputJayD() : btnPressCallbacks(btnNum, nullptr), btnReleaseCallbacks(btnNum, nullptr),
						 btnHoldCallbacks(btnNum, nullptr),
						 encMovedCallbacks(encNum, nullptr), potMovedCallbacks(potNum, nullptr),
						 btnHoldValue(btnNum, 0), btnHoldStart(btnNum, 0),
						 wasPressed(btnNum, false){

	Wire.begin(26, 27);

	instance = this;

}

void InputJayD::setBtnPressCallback(uint8_t id, void (*callback)()){
	btnPressCallbacks[id] = callback;
}

void InputJayD::setBtnReleaseCallback(uint8_t id, void (*callback)()){
	btnReleaseCallbacks[id] = callback;
}

void InputJayD::removeBtnPressCallback(uint8_t id){
	btnPressCallbacks[id] = nullptr;
}

void InputJayD::removeBtnReleaseCallback(uint8_t id){
	btnReleaseCallbacks[id] = nullptr;
}

InputJayD *InputJayD::getInstance(){
	return instance;
}

void InputJayD::setButtonHeldCallback(uint8_t id, uint32_t holdTime, void (*callback)()){
	btnHoldCallbacks[id] = callback;
	btnHoldValue[id] = holdTime;
}


void InputJayD::setEncoderMovedCallback(uint8_t id, void (*callback)(int8_t value)){
	encMovedCallbacks[id] = callback;
}

void InputJayD::removeEncoderMovedCallback(uint8_t id){
	encMovedCallbacks[id] = nullptr;
}

void InputJayD::setPotMovedCallback(uint8_t id, void (*callback)(uint8_t value)){
	potMovedCallbacks[id] = callback;
}

void InputJayD::removePotMovedCallback(uint8_t id){
	potMovedCallbacks[id] = nullptr;
}

uint8_t InputJayD::getNumEvents(){
	Wire.beginTransmission(deviceAddr);
	Wire.write(getEvents);
	Wire.endTransmission();
	Wire.requestFrom(deviceAddr, 2);
	if(Wire.available()){
		Wire.read();//addr
	}
	if(Wire.available()){
		uint8_t numEventsData = Wire.read();
		return numEventsData;
	}
}

void InputJayD::fetchEvents(int numEvents){
	if(numEvents == 0) return;
	Wire.beginTransmission(deviceAddr);
	Wire.write(sendEvents);
	Wire.write(numEvents);
	Wire.endTransmission();
	Wire.requestFrom(deviceAddr, 2 * numEvents + 1);
	if(Wire.available()){
		Wire.read();
	}
	std::vector<Event> events;
	uint8_t deviceId;
	int8_t valueData;
	for(int i = numEvents; i > 0; i--){
		if(Wire.available()){
			deviceId = Wire.read();
		}
		if(Wire.available()){
			valueData = Wire.read();

		}
		uint8_t id = deviceId & 0x0F;
		uint8_t device = deviceId >> 4;

		events.push_back({(DeviceType) device, id, valueData});
	}
	for(Event event:events){
		if(event.deviceType == BTN){
			handleButtonEvent(event.deviceID, event.value);
		}else if(event.deviceType == ENC){
			handleEncoderEvent(event.deviceID, event.value);
		}else if(event.deviceType == POT){
			handlePotentiometerEvent(event.deviceID, event.value);
		}
	}
}

void InputJayD::handleButtonEvent(uint8_t id, uint8_t value){
	if(value == 1){
		btnHoldStart[id] = millis();
		if(btnPressCallbacks[id] != nullptr){
			btnPressCallbacks[id]();
		}
	}

	if(value == 0){
		btnHoldStart[id] = 0;
		wasPressed[id] = false;

		if(btnReleaseCallbacks[id] != nullptr){
			btnReleaseCallbacks[id]();
		}
	}
}

void InputJayD::buttonHoldCheck(){
	for(uint8_t i = 0; i < btnHoldCallbacks.size(); i++){
		if(btnHoldCallbacks[i] == nullptr) continue;
		if(btnHoldStart[i] == 0) continue;
		if(millis() - btnHoldStart[i] >= btnHoldValue[i] && !wasPressed[i]){
			btnHoldCallbacks[i]();
			wasPressed[i] = true;
		}
	}
}

void InputJayD::handleEncoderEvent(uint8_t id, int8_t value){
	if(encMovedCallbacks[id] != nullptr){
		encMovedCallbacks[id](value);
	}
}

void InputJayD::handlePotentiometerEvent(uint8_t id, uint8_t value){
	if(potMovedCallbacks[id] != nullptr){
		potMovedCallbacks[id](value);
	}
}

void InputJayD::loop(uint _time){
	fetchEvents(getNumEvents());
	buttonHoldCheck();

}







