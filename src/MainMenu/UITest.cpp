#include <Loop/LoopManager.h>
#include "UITest.hpp"
#include <string>
#include <sstream>
#include "../Bitmaps/Image1.hpp"
#include "../Bitmaps/Image2.hpp"
#include "../Bitmaps/Image3.hpp"
#include "../Bitmaps/Image4.hpp"

const uint16_t *icons[] = {image1, image2, image3, image4};

UITest *UITest::instance = nullptr;

UITest::UITest(Display &display) : Context(display), menu(&screen, 2){
	instance = this;
	buildUI();

}


void UITest::start(){
	draw();
	screen.commit();

	InputJayD::getInstance()->setEncoderMovedCallback(1, [](int8_t value){
		if(value > 0){
			instance->selectNext();
		}else if(value < 0){
			instance->selectPrev();
		}
	});

}

void UITest::stop(){
	InputJayD::getInstance()->removeEncoderMovedCallback(1);
}

void UITest::draw(){
	screen.getSprite()->clear(TFT_BLACK);
	screen.draw();
}

void UITest::selectNext(){
	appIcons[selectedIcon].setSelected(false);
	selectedIcon = (selectedIcon + 1) % appIcons.size();
	appIcons[selectedIcon].setSelected(true);

	menu.draw();
	screen.commit();
}

void UITest::selectPrev(){
	appIcons[selectedIcon].setSelected(false);
	selectedIcon = selectedIcon == 0 ? appIcons.size() - 1 : selectedIcon - 1;
	appIcons[selectedIcon].setSelected(true);

	menu.draw();
	screen.commit();
}

void UITest::buildUI(){
	menu.setWHType(PARENT, FIXED);
	menu.setHeight(120);
	menu.setGutter(30);
	menu.setPadding(10);

	appIcons.reserve(sizeof(icons) /sizeof(icons[0]));//zahtjev za promjenu kapaciteta vektora, da bude velik barem onoliko koliko postoj elemenata u polju
	for(const auto &icon : icons){
		appIcons.emplace_back(&menu, icon);
		menu.addChild(&appIcons.back());

		if(appIcons.size() == 1) appIcons.back().setSelected(true);
	}

	menu.reflow();

	screen.addChild(&menu);
	screen.repos();

}


