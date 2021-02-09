#include "ScrollValue.hpp"

ScrollValue::ScrollValue(ElementContainer *parent):CustomElement(parent,10,10){

}

void ScrollValue::scrollValue(int8_t newValue){
	value = value+newValue;

}
void ScrollValue::draw(){

	getSprite()->setCursor(getTotalX(),value);

}
