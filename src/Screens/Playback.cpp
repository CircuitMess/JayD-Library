#include <SD.h>
#include <Loop/LoopManager.h>
#include "Playback.h"
#include "../AudioLib/AudioGeneratorWAV.h"
#include "../AudioLib/AudioOutputI2S.h"
#include "../InputLib/InputJayD.h"

#define I2S_WS 4
#define I2S_DO 16
#define I2S_BCK 17
#define I2S_DI -1

AudioGeneratorWAV* wav = nullptr;
AudioOutputI2S* i2s = nullptr;

Playback* Playback::instance = nullptr;

Playback::Playback(Display& display, const File& file) : Context(display), file(file){
	instance = this;
}

Playback::~Playback(){
	instance = nullptr;
}

void Playback::draw(){
	if(!file || wav == nullptr) return;

	Sprite* canvas = getScreen().getSprite();
	if(!canvas->created()){
		Serial.println("Canvas not created");
		return;
	}

	canvas->clear(TFT_BLACK);
	canvas->setTextFont(0);
	canvas->setTextSize(2);
	canvas->setTextColor(TFT_WHITE);

	canvas->setCursor(0, 3);
	canvas->printCenter(file.name()+1);

	int totalMins = wav->duration / 60;
	int totalSecs = wav->duration - totalMins * 60;
	int elapsedMins = wav->elapsed / 60;
	int elapsedSecs = wav->elapsed - elapsedMins * 60;
	char buffer[50];
	sprintf(buffer, "%d:%02d / %d:%02d", elapsedMins, elapsedSecs, totalMins, totalSecs);

	canvas->setTextSize(1);
	canvas->setCursor(0, 50);
	canvas->printCenter(buffer);
}

void Playback::start(){
	if(!file) return;

	wav = new AudioGeneratorWAV(&file);
	Serial.printf("Sample rate: %d, bit rate: %d\n", wav->getSampleRate(), wav->getBitsPerSample());

	const i2s_config_t i2s_config = {
			.mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX),
			.sample_rate = wav->getSampleRate(),
			.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
			.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
			.communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
			.intr_alloc_flags = 0,
			.dma_buf_count = 128,
			.dma_buf_len = 8,
			.use_apll = false

	};

	const i2s_pin_config_t pin_config = {
			.bck_io_num = I2S_BCK,
			.ws_io_num = I2S_WS,
			.data_out_num = I2S_DO,
			.data_in_num = I2S_DI
	};

	i2s = new AudioOutputI2S(i2s_config, pin_config, 0);
	i2s->setSource(wav);
	i2s->setGain(0.5);
	i2s->start();

	LoopManager::addListener(this);

	if(InputJayD::getInstance() == nullptr){
		return;
	}

	InputJayD::getInstance()->setPotMovedCallback(1, [](uint8_t val){
		if(instance == nullptr) return;
		Serial.println(val);

		i2s->setGain((float) val / 255.0f);
	});

	InputJayD::getInstance()->setBtnPressCallback(3, [](){
		if(instance == nullptr) return;
		instance->pop();
	});
}

void Playback::stop(){
	i2s->stop();

	delete i2s;
	delete wav;
	wav = nullptr;
	i2s = nullptr;


	LoopManager::removeListener(this);

	if(InputJayD::getInstance() != nullptr){
		InputJayD::getInstance()->removePotMovedCallback(1);
		InputJayD::getInstance()->removeBtnPressCallback(3);
	}
}

void Playback::loop(uint micros){
	if(i2s->isRunning()){
		i2s->loop(micros);
	}else{
		pop();
	}

	draw();
	screen.commit();
}
