#include <Arduino.h>
#include <CircuitOS.h>
#include "src/JayD.hpp"
#include <Loop/LoopManager.h>
#include "src/AudioLib/SourceWAV.h"
#include "src/AudioLib/OutputI2S.h"
#include "src/AudioLib/Mixer.h"
#include "src/PerfMon.h"
#include "src/Services/SDScheduler.h"
#include "src/Input/InputJayD.h"

#include <SPI.h>
#include <Devices/LEDmatrix/LEDmatrix.h>
#include <Devices/SerialFlash/SerialFlashFileAdapter.h>
#include <SD.h>
#include <WiFi.h>
#include <Util/Task.h>
#include <esp_partition.h>

LEDmatrixImpl LEDMatrix;

int pixel = 0;

void lightPixel(int pixel, bool turnoff = false){
	if(turnoff){
		LEDMatrix.drawPixel(::pixel, 0);
	}

	pixel = max(pixel, 0);
	pixel = min(pixel, 16 * 9 - 1);

	LEDMatrix.drawPixel(pixel, 255);

	::pixel = pixel;
}

void recurseDir(File dir){
	File f;
	while(f = dir.openNextFile()){
		if(f.isDirectory()){
			recurseDir(f);
			continue;
		}

		Serial.println(f.name());
		f.close();
	}
}

void listSD(){
	File root = SD.open("/");
	if(!root){
		Serial.println("List SD: can't open root");
		return;
	}

	recurseDir(root);
	root.close();
}

SourceWAV* wav1;
SourceWAV* wav2;
Mixer* mixer;

OutputI2S* i2s;

File f1, f2;

void mainThread(Task* t){
	if(!(f1 = SD.open("/Walter.wav"))){
		Serial.println("f1 error");
		for(;;);
	}
	if(!(f2 = SD.open("/Recesija.wav"))){
		Serial.println("f2 error");
		for(;;);
	}
	Serial.printf("Pre H: %d\n", ESP.getFreeHeap());
	wav1 = new SourceWAV(f1);
	Serial.printf("wav1 H: %d\n", ESP.getFreeHeap());
	wav2 = new SourceWAV(f2);
	Serial.printf("wav2 H: %d\n", ESP.getFreeHeap());
	mixer = new Mixer();
	Serial.printf("mixer H: %d\n", ESP.getFreeHeap());
	mixer->addSource(wav1);
	Serial.printf("mixer add 1 H: %d\n", ESP.getFreeHeap());
	mixer->addSource(wav2);
	Serial.printf("mixer add 2 H: %d\n", ESP.getFreeHeap());
	mixer->setMixRatio(170);


	mixer->setMixRatio(255/2);
	i2s->setSource(mixer);

	i2s->start();
	Serial.printf("I2S start: %d\n", ESP.getFreeHeap());

	for(;;){
		if(i2s->isRunning() && millis() < 10000){
			Profiler.init();
			i2s->loop(0);
			Profiler.report();
		}else{
			i2s->stop();
			for(;;);
		}
	}
}

void setup(){
	Serial.begin(115200);
	WiFi.mode(WIFI_OFF);
	btStop();

	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
		   chip_info.cores,
		   (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
		   (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

	printf("silicon revision %d, ", chip_info.revision);

	printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
		   (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

	fflush(stdout);

	if(psramFound()){
		Serial.printf("PSRAM init: %s, free: %d B\n", psramInit() ? "Yes" : "No", ESP.getFreePsram());
	}else{
		Serial.println("No PSRAM");
	}

	disableCore0WDT();
	disableCore1WDT();

	SPI.begin(18, 19, 23);
	SPI.setFrequency(60000000);
	if(!SD.begin(22, SPI)){
		Serial.println("No SD card");
		for(;;);
	}

	/*SPI.begin(18, 19, 23);
	SPI.setFrequency(60000000);
	if(!SD.begin(5, SPI)){
		Serial.println("No SD card");
		for(;;);
	}*/

	listSD();



	i2s = new OutputI2S({
								.mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX),
								.sample_rate = 44100,
								.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
								.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
								.communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
								.intr_alloc_flags = 0,
								.dma_buf_count = 6,
								.dma_buf_len = 512,
								.use_apll = false
						}, i2s_pin_config, I2S_NUM_0);

	Serial.printf("I2S: %d\n", ESP.getFreeHeap());
	i2s->setGain(0.1);

	//mainThread(nullptr);

	Task* maintask = new Task("Main", mainThread, 20240);
	maintask->start(1, 0);

	for(;;){
		Sched.loop(0);
	}

	vTaskDelete(nullptr);

	/*InputJayD* input = new InputJayD();
	input->begin();
	LoopManager::addListener(input);

	digitalWrite(JDNV_PIN_RESET, LOW);*/

	/*LEDMatrix.begin(26, 27);
	LEDMatrix.clear();
	LEDMatrix.push();

	InputJayD::getInstance()->setEncoderMovedCallback(1, [](int8_t value){
		lightPixel(pixel + value, true);
		LEDMatrix.push();

		Serial.printf("%d\n", pixel);
	});*/
}

int i = 0;
void loop(){
	LoopManager::loop();

	/*i++;

	LEDMatrix.clear();
	lightPixel((i/13) % 144);
	lightPixel((i/17) % 144);
	lightPixel((i/5) % 144);
	lightPixel((i/7) % 144);
	lightPixel((i/11) % 144);
	LEDMatrix.push();*/
}