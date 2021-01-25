#include "InputJayD.h"
#include <Wire.h>
#include <Arduino.h>
#include <driver/i2s.h>


InputJayD *InputJayD::instance;

InputJayD::InputJayD() : btnPressCallbacks(btnNum, nullptr), btnReleaseCallbacks(btnNum, nullptr),
						 btnHoldRepeatCallbacks(btnNum, nullptr), btnHoldCallbacks(btnNum, nullptr),
						 encMovedCallbacks(btnNum, nullptr),
						 btnHoldValue(btnNum, 0), btnHoldStart(btnNum, 0),
						 btnState(btnNum, 0){

	Wire.begin(26, 27);

	instance = this;

}

void InputJayD::setBtnPressCallback(uint8_t _id, void (*callback)()){
	btnPressCallbacks[_id] = callback;
}

void InputJayD::setBtnReleaseCallback(uint8_t _id, void (*callback)()){
	btnReleaseCallbacks[_id] = callback;
}

void InputJayD::removeBtnPressCallback(uint8_t _id){
	btnPressCallbacks[_id] = nullptr;
}

void InputJayD::removeBtnReleaseCallback(uint8_t _id){
	btnReleaseCallbacks[_id] = nullptr;
}

InputJayD *InputJayD::getInstance(){
	return instance;
}

void InputJayD::setButtonHeldCallback(uint8_t _id, uint32_t holdTime, void (*callback)()){
	btnHoldCallbacks[_id] = callback;
	btnHoldValue[_id] = holdTime;
}


void InputJayD::setEncoderMovedCallback(uint8_t _id, void (*callback)(int8_t value)){
	encMovedCallbacks[_id] = callback;
}

void InputJayD::removeEncoderMovedCallback(uint8_t _id){
	encMovedCallbacks[_id] = nullptr;
}

void InputJayD::setPotMovedCallback(uint8_t _id, void (*callback)(uint8_t value)){
	potMovedCallbacks[_id] = callback;
}

void InputJayD::removePotMovedCallback(uint8_t _id){
	potMovedCallbacks[_id] = nullptr;
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
	for(int i = numEvents; i > 0; i--){
		if(Wire.available()){
			deviceId = Wire.read();
		}
		if(Wire.available()){
			valueData = Wire.read();
		}
		id = deviceId & 0x0F;
		device = deviceId >> 4;

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

void InputJayD::handleButtonEvent(uint8_t _id, uint8_t _value){
	if(btnPressCallbacks[_id] != nullptr){
		if(_value == 1){
			btnPressCallbacks[_id]();
		}
	}
	if(btnHoldCallbacks[_id] != nullptr){
		if(_value == 1){
			btnHoldStart[_id] = millis();
			if(btnHoldStart[_id] > btnHoldValue[_id]){
				btnHoldCallbacks[_id]();
			}
		}
	}
	if(btnReleaseCallbacks[_id] != nullptr){
		if(_value == 0){
			btnHoldStart[_id] = 0;
			btnReleaseCallbacks[_id]();
		}

	}
}

void InputJayD::handleEncoderEvent(uint8_t _id, uint8_t _value){
	if(encMovedCallbacks[_id] != nullptr){
		encMovedCallbacks[_id](_value);
	}
}

void InputJayD::handlePotentiometerEvent(uint8_t _id, uint8_t _value){
	if(potMovedCallbacks[_id] != nullptr){
		potMovedCallbacks[_id](_value);
	}
}

void InputJayD::loop(uint _time){
	fetchEvents(getNumEvents());

}







