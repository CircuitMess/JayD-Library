#include <Arduino.h>
#include <CircuitOS.h>
#include "src/JayD.hpp"
#include <Loop/LoopManager.h>

#include <SPI.h>
#include <Devices/SerialFlash/SerialFlashFileAdapter.h>
#include <SD.h>
#include <Network/Net.h>

#include <Network/StreamableHTTPClient.h>

#define CA "DC:03:B5:D6:0C:F1:02:F1:B1:D0:62:27:9F:3E:B4:C3:CD:C9:93:BA:20:65:6D:06:DC:5D:56:AC:CC:BA:40:20"
StreamableHTTPClient http;
File* file;
struct wavHeader{
	char RIFF[4];
	uint32_t chunkSize;
	char WAVE[4];
	char fmt[3];
	uint32_t fmtSize;
	uint16_t audioFormat;
	uint16_t numChannels;
	uint32_t sampleRate;
	uint32_t byteRate; // == SampleRate * NumChannels * BitsPerSample/8
	uint16_t blockAlign; // == NumChannels * BitsPerSample/8
	uint16_t bitsPerSample;
	char data[4];
	uint32_t dataSize; // == NumSamples * NumChannels * BitsPerSample/8
};

#define I2S_WS 4
#define I2S_DO 16
#define I2S_BCK 17
#define I2S_DI -1

const i2s_config_t i2s_config = {
		.mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX),
		.sample_rate = 44100,
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

AudioOutput* output;

void send(){
	size_t length = file->size();
	http.useHTTP10(true);
	http.setReuse(false);
	if(!http.begin("http://spencer.circuitmess.com:7979/do.php")){
		Serial.println("bnegin error");
		return;
	}
	http.addHeader("Accept-Encoding", "identity");
	http.addHeader("Content-Length", String(length));

	if(!http.startPOST()){
		Serial.println("Error connecting");
		http.end();
		http.getStream().stop();
		http.getStream().flush();
		return;
	}

	wavHeader header;
	memcpy(header.RIFF, "RIFF", 4);
	header.chunkSize = length - 8;
	memcpy(header.WAVE, "WAVE", 4);
	memcpy(header.fmt, "fmt ", 4);
	header.fmtSize = 16;
	header.audioFormat = 1; //PCM
	header.numChannels = DEFAULT_NUMCHANNELS; //2 channels
	header.sampleRate = DEFAULT_SAMPLERATE;
	header.byteRate = DEFAULT_SAMPLERATE * DEFAULT_NUMCHANNELS * DEFAULT_BYTESPERSAMPLE;
	header.blockAlign = DEFAULT_NUMCHANNELS * DEFAULT_BYTESPERSAMPLE;
	header.bitsPerSample = DEFAULT_BYTESPERSAMPLE * 8;
	memcpy(header.data, "data", 4);
	header.dataSize = length - 44;

	http.send((uint8_t*)&header, sizeof(wavHeader));


	output = new AudioOutputI2S(i2s_config, pin_config, 0, &http);
	output->setSource(new AudioGeneratorWAV(file));
	output->setGain(0.5);
	LoopManager::addListener(output);
}
void setup(){
	Serial.begin(115200);

	SPI.begin(18, 19, 23);
	SPI.setFrequency(60000000);

	if(!SD.begin(22)){
		Serial.println("SD card fail");
	}



	file = new File(SD.open("/Darude.wav", "r"));
	Net.set("CircuitMess", "MAKERphone!");
	Net.connect([](wl_status_t status){
		if(status == WL_CONNECTED){
			Serial.println("sending");
			send();
		}else{
			Serial.println("network error");
			for(;;){
			}
		}
	});


}

void loop(){
	LoopManager::loop();
}