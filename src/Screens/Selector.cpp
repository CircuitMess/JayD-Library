#include <SD.h>
#include <Loop/LoopManager.h>
#include "Selector.h"
#include "../InputLib/InputJayD.h"

Selector* Selector::instance = nullptr;

Selector::Selector(Display& display) : Context(display), menu(&getScreen(), "SD card"){
	instance = this;

	getScreen().addChild(&menu);
	menu.setWHType(PARENT, PARENT);
	menu.reflow();
	menu.repos();
}

Selector::~Selector(){
	instance = nullptr;
}

void Selector::draw(){
	screen.draw();
}

void Selector::start(){
	delete playback;
	playback = nullptr;

	menu.clearItems();

	File root = SD.open("/");
	File f;

	while(f = root.openNextFile()){
		if(f.isDirectory()) continue;

		String name = f.name();
		name.toLowerCase();
		if(!name.endsWith(".wav")) continue;

		char* cname = static_cast<char*>(malloc(name.length()));
		memcpy(cname, f.name()+1, name.length()-1);
		memset(cname + name.length() - 1, 0, 1);

		menu.addItem(cname);
	}

	menu.reflow();
	menu.repos();

	draw();
	screen.commit();

	if(InputJayD::getInstance() == nullptr){
		LoopManager::addListener(this);
		return;
	}

	InputJayD::getInstance()->setEncoderMovedCallback(1, [](int8_t amount){
		if(instance == nullptr) return;

		bool pos = amount > 0;
		for(int i = 0; i < abs(amount); i++){
			if(pos){
				instance->menu.selectNext();
			}else{
				instance->menu.selectPrev();
			}
		}

		instance->draw();
		instance->screen.commit();
	});

	InputJayD::getInstance()->setBtnPressCallback(3, [](){
		if(instance == nullptr) return;

		Serial.println("BTN press");
		Serial.println("/" + String(instance->menu.getSelectedItem().title));
		File f = SD.open("/" + String(instance->menu.getSelectedItem().title));
		Serial.println(f ? "open" : "not open");

		instance->playback = new Playback(*instance->getScreen().getDisplay(), f);
		instance->playback->push(instance);
	});
}

void Selector::stop(){
	if(InputJayD::getInstance() == nullptr){
		LoopManager::removeListener(this);
	}else{
		InputJayD::getInstance()->removeEncoderMovedCallback(1);
		InputJayD::getInstance()->removeBtnPressCallback(3);
	}
}

int played = 0;

void Selector::loop(uint dt){
	if(millis() / 1000 <= step) return;

	menu.selectNext();
	draw();
	screen.commit();

	if(menu.getSelected() == (2 + played) % 8 && step > 4){
		played++;
		playback = new Playback(*getScreen().getDisplay(), SD.open("/" + String(menu.getSelectedItem().title)));
		playback->push(this);
		return;
	}

	step = (millis() / 1000);
}
