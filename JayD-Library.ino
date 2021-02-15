#include <Arduino.h>
#include <CircuitOS.h>
#include "src/JayD.h"
#include "src/InputLib/InputJayD.h"
#include "src/Screens/Selector.h"
#include "src/Screens/Playback.h"
#include <Loop/LoopManager.h>
#include <Display/Display.h>
#include <Support/Context.h>
#include <SD.h>
#include "src/snow.hpp"
#include <Devices/LEDmatrix/LEDmatrix.h>
#include <Util/Task.h>

Display display(160, 128, -1, -3);
LEDmatrixImpl LEDMatrix;

void MatrixTask(Task* task){
	InputJayD* input = new InputJayD();
	input->begin();

	uint32_t lastMicros = micros();

	while(task->running){
		uint32_t m = micros();
		uint32_t dt = m - lastMicros;

		LEDMatrix.loop(dt);
		if(InputJayD::getInstance() != nullptr){
			InputJayD::getInstance()->loop(dt);
		}

		lastMicros = m;
	}
}

void setup(){
	Serial.begin(115200);

	SPI.begin(18, 19, 23);
	SPI.setFrequency(60000000);

	pinMode(25, OUTPUT);
	digitalWrite(1, 25);
	display.begin();

	if(!SD.begin(22)){
		Serial.println("SD card fail");
	}

	LEDMatrix.begin(26, 27);
	File f = SD.open("/snow.gif");
	if(!f){
		Serial.println("No snow!");
	}else{
		LEDMatrix.startAnimation(new Animation(&f), true);
	}
	Task* lmTask = new Task("LedMatrix", MatrixTask, 2048, nullptr);
	//lmTask->start(1, 0);
	LoopManager::addListener(&LEDMatrix);
	InputJayD* input = new InputJayD();
	input->begin();
	LoopManager::addListener(input);

	Context* selector = new Selector(display);

	selector->unpack();
	selector->start();

	Serial.printf("ID is %s\n", input->identify() ? "OK" : "not OK");
	Serial.println("resetting");
	delay(100);
	Serial.printf("ID is %s\n", input->identify() ? "OK" : "not OK");
}

void loop(){
	LoopManager::loop();
}