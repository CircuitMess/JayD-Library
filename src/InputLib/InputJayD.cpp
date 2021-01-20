#include "InputJayD.h"
#include <Wire.h>
#include <Arduino.h>
#include <driver/i2s.h>


InputJayD *InputJayD::instance;

InputJayD::InputJayD() : btnPressCallback(id, nullptr), btnReleaseCallback(id, nullptr),
						 btnHoldRepeatCallback(id, nullptr), btnHoldCallback(id, nullptr), encMovedCallback(id, 0),
						 btnHoldValue(id, 0), btnHoldRepeatValue(id, 0), btnHoldStart(id, 0), btnHoldOver(id, 0){

	Wire.begin(26, 27);

	instance = this;

}

void InputJayD::setBtnPressCallback(uint8_t _id, void (*callback)()){
	btnPressCallback[_id] = callback;
}

void InputJayD::setBtnReleaseCallback(uint8_t _id, void (*callback)()){
	btnReleaseCallback[_id] = callback;
}

void InputJayD::removeBtnPressCallback(uint8_t _id){
	btnPressCallback[_id] = nullptr;
}

void InputJayD::removeBtnReleaseCallback(uint8_t _id){
	btnReleaseCallback[_id] = nullptr;
}

InputJayD *InputJayD::getInstance(){
	return instance;
}

void InputJayD::setButtonHeldCallback(uint8_t _id, uint32_t holdTime, void (*callback)()){
	btnHoldCallback[_id] = callback;
	btnHoldValue[_id] = holdTime;
}

void InputJayD::setButtonHeldRepeatCallback(uint8_t _id, uint32_t periodTime, void (*callback)(uint)){
	btnHoldRepeatCallback[_id] = callback;
	btnHoldRepeatValue[_id] = periodTime;
}

uint32_t InputJayD::getButtonMillis(uint8_t _id){
	return millis() - btnHoldStart[_id];
}


void InputJayD::setEncoderMovedCallback(uint8_t _id, void (*callback)(int8_t value)){
	encMovedCallback[_id] = callback;
}

void InputJayD::removeEncoderMovedCallback(uint8_t _id){
	encMovedCallback[_id] = nullptr;
}

void InputJayD::setPotMovedCallback(uint8_t _id, void (*callback)(uint8_t value)){
	potMovedCallback[_id] = callback;
}

void InputJayD::removePotMovedCallback(uint8_t _id){
	potMovedCallback[_id] = nullptr;
}

void InputJayD::loop(uint _time){
	Wire.beginTransmission(deviceAddr);
	Wire.write(getEvents);
	Wire.endTransmission();
	//Wire.write(0x12);
	Wire.requestFrom(deviceAddr, 2);
	if(Wire.available()){
		numEventsAddr = Wire.read();
		Serial.println("slaveAddr");
		Serial.println(numEventsAddr);
	}
	if(Wire.available()){
		numEventsData = Wire.read();
		Serial.println("numEvents");
		Serial.println(numEventsData);
	}
	if(numEventsData > 0){
		Wire.beginTransmission(deviceAddr);
		Wire.write(sendEvents);
		Wire.write(numEventsData);
		Wire.endTransmission();

		Wire.requestFrom(deviceAddr, 2 * numEventsData + 1);
		if(Wire.available()){
			outputAddr = Wire.read();
			Serial.println("outputAddr");
			Serial.println(outputAddr);
		}
		for(int i = numEventsData; i > 0; i--){
			if(Wire.available()){
				deviceId = Wire.read();
				if(deviceId != temp){
					temp = deviceId;
					Serial.println("ovo je trenutni podatak");
					Serial.println(temp);
				}
				//	Serial.println("deviceID");
				//Serial.println(deviceId);
			}
			if(Wire.available()){
				valueData = Wire.read();
				Serial.println("outputData");
				Serial.println(valueData);
			}

			id = deviceId & 0x0F;
			Serial.println("id:");
			Serial.println(id);
			device = deviceId >> 4;
			Serial.println("device:");
			Serial.println(device);

			/*if(device == 0){
				if(btnPressCallback[id] != nullptr){
					btnPressCallback[id]();
					btnHoldStart[id] = millis();
					if(btnHoldValue[id]==getButtonMillis(id)){
						btnPressCallback[id]();
					}
				}else{
					btnReleaseCallback[id]();
				}

			}else if(device == 1){
				if(encMovedCallback[id] != 0){
					encMovedCallback[id](valueData);
				}
			}else if(device == 2){
				if(potMovedCallback[id] != 0){
					potMovedCallback[id](valueData);
				}
			}*/
		}

	}

}



