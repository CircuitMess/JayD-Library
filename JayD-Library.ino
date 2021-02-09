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

void draw();

void buttonLeftPress();

void buttonLeftRelease();

void buttonRightPress();

void buttonRightRelease();

void buttonENC8Press();

void buttonENC8Release();

void valueEnc(int8_t value);

void valuePot0(uint8_t value);

void valuePot1(uint8_t value);

void valuePot2(uint8_t value);

bool leftState = false;
bool rightState = false;
int enc8State = 0;
uint8_t potVal0 = 0;
uint8_t potVal1 = 0;
uint8_t potVal2 = 0;

void setup(){
	Serial.begin(115200);
	pinMode(blPin, OUTPUT);
	digitalWrite(blPin, LOW);
	display.begin();
	jayDinput = new InputJayD();
	Serial.println("Print1");
	LoopManager::addListener(jayDinput);
	Serial.println("Print2");
	uiTest=new UITest(display);
	Serial.println("Print3");
	uiTest->unpack();
	uiTest->start();
	Serial.println("Print4");





	/*leDmatrix = new LEDmatrixImpl(16, 9);
	leDmatrix->begin(26, 27);
	leDmatrix->clear();
	leDmatrix->setBrightness(100);
	for(uint8_t x = 0; x < 16; x++){
		for(uint8_t y = 0; y < 9; y++){
			leDmatrix->drawPixel(x, y, 100);

		}
	}
	leDmatrix->push();*/

	//
	//jayDinput->getInstance()->setBtnPressCallback(0, buttonLeftPress);
//	jayDinput->getInstance()->setBtnReleaseCallback(0, buttonLeftRelease);
//	jayDinput->getInstance()->setBtnPressCallback(1, buttonRightPress);
//	jayDinput->getInstance()->setBtnReleaseCallback(1, buttonRightRelease);
	//jayDinput->getInstance()->setButtonHeldCallback(8, 3000, buttonENC8Press);
	//jayDinput->getInstance()->setBtnReleaseCallback(8, buttonENC8Release);
//	jayDinput->getInstance()->setEncoderMovedCallback(2, valueEnc);
	/*jayDinput->getInstance()->setPotMovedCallback(0, valuePot0);
	jayDinput->getInstance()->setPotMovedCallback(1, valuePot1);
	jayDinput->getInstance()->setPotMovedCallback(2, valuePot2);*/
	//jayD=new JayD(display,jayDinput);
	//LoopManager::addListener(jayDinput);
	//LoopManager::addListener(jayD);

}

void loop(){

	LoopManager::loop();
}


void draw(){

	/*if(leftState){
		display.getBaseSprite()->clear(TFT_DARKGREY);
		display.getBaseSprite()->setTextFont(1);
		display.getBaseSprite()->setTextColor(TFT_WHITE);
		display.getBaseSprite()->setTextSize(3);
		display.getBaseSprite()->setCursor(40, 50);
		display.getBaseSprite()->println("JayD1");
	}if(!leftState){
		display.getBaseSprite()->clear(TFT_DARKGREY);
		display.getBaseSprite()->setTextFont(1);
		display.getBaseSprite()->setTextColor(TFT_WHITE);
		display.getBaseSprite()->setTextSize(3);
		display.getBaseSprite()->setCursor(40, 50);
		display.getBaseSprite()->println("false1");
	}*/
	/*if(rightState){
		display.getBaseSprite()->clear(TFT_DARKGREY);
		display.getBaseSprite()->setTextFont(1);
		display.getBaseSprite()->setTextColor(TFT_WHITE);
		display.getBaseSprite()->setTextSize(3);
		display.getBaseSprite()->setCursor(40, 50);
		display.getBaseSprite()->println("JayD2");
	}if(!rightState){
		display.getBaseSprite()->clear(TFT_DARKGREY);
		display.getBaseSprite()->setTextFont(1);
		display.getBaseSprite()->setTextColor(TFT_WHITE);
		display.getBaseSprite()->setTextSize(3);
		display.getBaseSprite()->setCursor(40, 50);
		display.getBaseSprite()->println("false2");
	}*/



	display.getBaseSprite()->clear(TFT_DARKGREY);
	display.getBaseSprite()->setTextFont(1);
	display.getBaseSprite()->setTextColor(TFT_WHITE);
	display.getBaseSprite()->setTextSize(2);
	display.getBaseSprite()->setCursor(40, 5);
	display.getBaseSprite()->println(potVal0);
	display.getBaseSprite()->setCursor(40, 30);
	display.getBaseSprite()->println(potVal1);
	display.getBaseSprite()->setCursor(40, 60);
	display.getBaseSprite()->println(potVal2);


	/*if(!enc8State){
		display.getBaseSprite()->clear(TFT_DARKGREY);
		display.getBaseSprite()->setTextFont(1);
		display.getBaseSprite()->setTextColor(TFT_WHITE);
		display.getBaseSprite()->setTextSize(3);
		display.getBaseSprite()->setCursor(40, 50);
		display.getBaseSprite()->println("left");
	}
*/

}

void buttonLeftPress(){
	leftState = true;
}

void buttonLeftRelease(){
	leftState = false;
}

void buttonRightPress(){
	rightState = true;
}

void buttonRightRelease(){
	rightState = false;
}

void buttonENC8Press(){
	enc8State = true;
}

void buttonENC8Release(){
	enc8State = false;
}

void valueEnc(int8_t value){
	if(value > 0){
		enc8State++;
	}else if(value < 0){
		enc8State--;
	}
}

void valuePot0(uint8_t value){
	potVal0 = value;
}

void valuePot1(uint8_t value){
	potVal1 = value;
}

void valuePot2(uint8_t value){
	potVal2 = value;
}




