#include "BtnValue.hpp"

BtnValue::BtnValue(ElementContainer *parent) : CustomElement(parent, 10, 10){


}

void BtnValue::buttonTap(){
	btnValue=btnValue+1;
}
void BtnValue::draw(){
	getSprite()->setTextFont(1);
	getSprite()->setTextColor(TFT_WHITE);
	getSprite()->setTextSize(2);
	getSprite()->setCursor(getTotalX(), getTotalY());
	getSprite()->println(btnValue);
}