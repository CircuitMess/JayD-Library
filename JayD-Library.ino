#include <Arduino.h>
#include <CircuitOS.h>
#include "src/JayD.hpp"
#include <Loop/LoopManager.h>

#include <SPI.h>
#include <SerialFlash.h>
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

const i2s_config_t i2s_config = {
	.mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX), // Receive and transfer
	.sample_rate = 16000,                         // 44.1KHz
	.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // could only get it to work with 32bits
	.channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT, // although the SEL config should be left, it seems to transmit on right
	.communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
	.intr_alloc_flags = 0,     // Interrupt level 1
	.dma_buf_count = 16,                           // number of buffers
	.dma_buf_len = 60,                              // 60 samples per buffer (8 minimum)
	.use_apll = false
};

// The pin config as per the setup
const i2s_pin_config_t ringo_pin_config = {
	.bck_io_num = 13,   // BCKL
	.ws_io_num = 15,    // LRCL
	.data_out_num = 22,	//speaker pin
	.data_in_num = 33	//mic pin
};


AudioOutput* output;
void setup(){
	Serial.begin(115200);

	//RINGO TESTING
	//-------------------------------------------
	pinMode(32, OUTPUT);
	digitalWrite(32, LOW);
	pinMode(26, OUTPUT);
	digitalWrite(26, 0);
	pinMode(36, INPUT_PULLUP);
	pinMode(34, INPUT_PULLUP);
	pinMode(33, INPUT_PULLUP);
	pinMode(39, INPUT_PULLUP);
	pinMode(25, OUTPUT);
	digitalWrite(25, 0);
	while (!SD.begin(5, SPI, 8000000)){
		Serial.println("SD ERROR");
	}


	file = new File(SD.open("/song.wav", "r"));
	Net.set("ssid", "pass");
	Net.connect([](wl_status_t status){
		if(status == WL_CONNECTED){
			output->setSource(new AudioGeneratorWAV(file));
		}
	});

	size_t length = file->size();
	http.useHTTP10(true);
	http.setReuse(false);
	if(!http.begin("", CA)){
		return;
	}
	http.addHeader("Content-Type", "application/json; charset=utf-8");
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


	output = new AudioOutputI2S(i2s_config, ringo_pin_config, 0, &http);
	LoopManager::addListener(output);
	output->setGain(0.5);
}

void loop(){
	LoopManager::loop();
}