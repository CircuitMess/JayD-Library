#include "AppIcon.hpp"

AppIcon::AppIcon(ElementContainer *parent, const uint16_t *icon):CustomElement(parent,30,30), icon(icon){

}
void AppIcon::draw(){

	getSprite()->fillRect(getTotalX(),getTotalY(),35,35,TFT_WHITE);
	getSprite()->drawIcon(icon,getTotalX()+5,getTotalY()+5,30,30,1,TFT_TRANSPARENT);

	Element::draw();
}
void AppIcon::setSelected(bool selected){
	AppIcon::selected = selected;
}