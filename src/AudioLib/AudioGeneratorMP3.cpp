#include "AudioGeneratorMP3.h"

AudioGeneratorMP3::AudioGeneratorMP3() : file(nullptr), channels(0), sampleRate(0), bitsPerSample(0), decoder(OpenMP3::Decoder(openmp3)){
}

AudioGeneratorMP3::AudioGeneratorMP3(fs::File *_file) : AudioGeneratorMP3(){
	file = _file;
}

AudioGeneratorMP3::~AudioGeneratorMP3(){
}

int AudioGeneratorMP3::generate(int16_t *outBuffer){

	Serial.println("generate start");
	delay(6);
	if(file == nullptr){
		Serial.println("file nullptr");
		return 0;
	}
	if(!file){
		Serial.println("file false");
		return 0;
	}

	file->read((uint8_t*)outBuffer, 100*sizeof(int16_t));
	Serial.println("file->read done");
	delay(6);
	OpenMP3::Iterator itr(openmp3, (OpenMP3::UInt8*)outBuffer, 100*sizeof(int16_t));
	float buffer[2][1152];
	std::vector <float> channels[2];
	bool mono = true;
	Serial.println("mp3 init done");
	delay(6);

	itr.GetNext(frame);
	Serial.println("itr.GetNext()");
	delay(6);

	OpenMP3::UInt nsamples = decoder.ProcessFrame(frame, buffer);
	// Serial.print("samples to be read: ");
	// Serial.println(nsamples);
	// delay(6);

	// for(int ch = 0; ch < 2; ++ch){
	// 	auto & channel = channels[ch];
	// 	auto * data = buffer[ch];
	// 	for (OpenMP3::UInt idx = 0; idx < nsamples; ++idx) channel.push_back(*data++);
	// }
	// Serial.println("decoding done");
	// delay(6);
	// mono = mono && (frame.GetMode() == OpenMP3::kModeMono);
	// OpenMP3::UInt length = channels[0].size();


	return 0;
}

int AudioGeneratorMP3::getBitsPerSample(){
	return bitsPerSample;
}

int AudioGeneratorMP3::getSampleRate(){
	return sampleRate;
}

int AudioGeneratorMP3::getChannels(){
	return channels;
}

void AudioGeneratorMP3::open(fs::File *_file){
	if(file != nullptr) delete file;
	
	file = _file;
	channels, sampleRate, bitsPerSample = 0;
}

int AudioGeneratorMP3::available(){
	return file->available();
}