#include "SourceMP3.h"

SourceMP3::SourceMP3(){
}

SourceMP3::SourceMP3(fs::File *_file) : SourceMP3(){
	file = _file;
}

SourceMP3::~SourceMP3(){
}

size_t SourceMP3::generate(int16_t *outBuffer){

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

	return 0;
}

void SourceMP3::open(fs::File *_file){
	if(file != nullptr) delete file;
	
	file = _file;
	channels, sampleRate, bytesPerSample = 0;
}

int SourceMP3::available(){
	return file->available();
}