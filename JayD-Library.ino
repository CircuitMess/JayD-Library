#include <Arduino.h>
#include <CircuitOS.h>
#include <TFT_eSPI.h>
#include <Display/Display.h>
#include <Input/InputGPIO.h>
#include <Input/Input.h>
#include <Loop/LoopListener.h>
#include <Loop/LoopManager.h>
#include <esp_err.h>
#include "src/InputLib/InputJayD.h"
#include "src/MainMenu/UITest.hpp"


#define blPin 25


Display display(160, 128, -1, -1);
//JayD *jayD;
InputJayD *jayDinput;
UITest* uiTest;

void setup(){
	Serial.begin(115200);
	Serial.println("ino");
	pinMode(blPin, OUTPUT);
	digitalWrite(blPin, LOW);
	display.begin();
	jayDinput = new InputJayD();
	LoopManager::addListener(jayDinput);
	uiTest=new UITest(display);
	uiTest->unpack();
	uiTest->start();

}

void loop(){

	LoopManager::loop();
}







