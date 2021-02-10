#include "AppIcon.hpp"

AppIcon::AppIcon(ElementContainer *parent, const uint16_t *icon) : CustomElement(parent, 53, 30), icon(icon){

}

void AppIcon::draw(){

	getSprite()->fillRect(getTotalX(), getTotalY(), 63, 45, TFT_DARKGREY);
	getSprite()->fillRect(getTotalX() + 2,getTotalY() + 2,60,40,selected ? TFT_RED : TFT_DARKGREY);
	getSprite()->drawIcon(icon, getTotalX() + 5, getTotalY() + 5, 53, 35, 1, TFT_TRANSPARENT);
	Element::draw();
}

void AppIcon::setSelected(bool selected){
	AppIcon::selected = selected;
}