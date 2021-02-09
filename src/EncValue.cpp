#include <sstream>
#include "EncValue.hpp"


EncValue::EncValue(ElementContainer *parent) : CustomElement(parent, 10, 10){

}

void EncValue::encoderMove(int8_t newValue){
	value = value + newValue;

}

void EncValue::draw(){

	getSprite()->setTextFont(1);
	getSprite()->setTextColor(TFT_WHITE);
	getSprite()->setTextSize(2);
	getSprite()->setCursor(getTotalX(), getTotalY());
	getSprite()->println(value);

}