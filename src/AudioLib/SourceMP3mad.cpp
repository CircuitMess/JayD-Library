#include "SourceMP3mad.h"
#include "Decoder/mad/synth.h"
#include "../AudioSetup.hpp"
#include "../PerfMon.h"
#include <SD.h>

SourceMP3mad::SourceMP3mad(const String& path){
	open(path);
}

SourceMP3mad::~SourceMP3mad(){
	delete input;
}

void SourceMP3mad::open(File& file){
	this->file = file;

	ID3Parser parser(file);
	metadata = parser.parse();
	if(metadata.headerSize == 0){
		Serial.println("Failed parsing MP3 header");
		return;
	}

	file.seek(metadata.headerSize);
	Serial.printf("Header size %d\n", metadata.headerSize);

	dataStart = 0;

	input = new FSBuffer(file, MP3_BUFFERSIZE);
	input->refill();

	stream = static_cast<mad_stream*>(malloc(sizeof(mad_stream)));
	frame = static_cast<mad_frame*>(malloc(sizeof(mad_frame)));
	synth = static_cast<mad_synth*>(malloc(sizeof(mad_synth)));

	mad_stream_init(stream);
	mad_frame_init(frame);
	mad_synth_init(synth);

	output = new DataBuffer(DATA_BUFFERSIZE);

	samplesRead = samplesAvailable = 0;
	decode();
	bytesPerSample = BYTES_PER_SAMPLE;
}

size_t SourceMP3mad::generate(int16_t* outBuffer){
	if(!file){
		Serial.println("file false");
		return 0;
	}

	Profiler.start("MP3 decode");
	while(output->readAvailable() < BUFFER_SIZE){
		decode();
	}
	Profiler.end();

	memcpy(outBuffer, output->readData(), BUFFER_SIZE);
	output->readMove(BUFFER_SIZE);

	return BUFFER_SAMPLES;
}

void SourceMP3mad::decode(){
	// Process remaining samples
	processSynth();

	// If still couldn't process all of them
	if(samplesRead != samplesAvailable){
		return;
	}

	// Refill input buffer
	// if(input->available() < 900 /* kolko ide madu */){
		// input->refill();
	// }

	//input->moveRead(input->available());
	if(stream->next_frame){
		input->moveRead(stream->next_frame - stream->buffer);
	}else{
		//input->moveRead(input->available());
	}
	input->refill();
	mad_stream_buffer(stream, input->data(), input->available());

	for(;;){
		int ret = mad_frame_decode(frame, stream);
		// input->moveRead(stream->next_frame - stream->buffer);

		if(ret == -1){
			if(!MAD_RECOVERABLE(stream->error)){
				break;
			}

			Serial.printf("Unrecoverable error %d\n", stream->error);
			continue;
		}

		mad_synth_frame(synth, frame, nullptr, nullptr);
		sampleRate = frame->header.samplerate;
		channels = MAD_NCHANNELS(&frame->header);

		samplesAvailable = synth->pcm.length; // broj samplova
		samplesRead = 0;

		processSynth();
	}
}

void SourceMP3mad::processSynth(){
	if(samplesRead == samplesAvailable) return;

	int dataPtr = 0;
	int i;
	for(i = samplesRead; i < samplesAvailable && ((i+1) * bytesPerSample * channels) <= output->writeAvailable(); i++){
		for(int j = 0; j < channels; j++){
			if(j != 0) continue;
			reinterpret_cast<int16_t*>(output->writeData())[dataPtr++] = synth->pcm.samples[j][i];
			Serial.printf("%d\n", synth->pcm.samples[j][i]);
		}
	}

	output->writeMove(dataPtr * bytesPerSample);
	samplesRead = i;
}

void SourceMP3mad::open(const String& path){
	File f = openUnicodePath(path.c_str());
	open(f);
}

SourceMP3mad::SourceMP3mad(File& file){
	open(file);
}

File SourceMP3mad::openUnicodePath(const char* filepath){
	size_t len = strlen(filepath) + 1;
	char* path = static_cast<char*>(malloc(len));
	memcpy(path, filepath, len);

	String s_file = filepath;
	const uint8_t ascii[60] = {
			//129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148  // UTF8(C3)
			//                Ä    Å    Æ    Ç         É                                       Ñ                  // CHAR
			000, 000, 000, 142, 143, 146, 128, 000, 144, 000, 000, 000, 000, 000, 000, 000, 165, 000, 000, 000, // ASCII
			//149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168
			//      Ö                             Ü              ß    à                   ä    å    æ         è
			000, 153, 000, 000, 000, 000, 000, 154, 000, 000, 225, 133, 000, 000, 000, 132, 134, 145, 000, 138,
			//169, 170, 171, 172. 173. 174. 175, 176, 177, 179, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188
			//      ê    ë    ì         î    ï         ñ    ò         ô         ö              ù         û    ü
			000, 136, 137, 141, 000, 140, 139, 000, 164, 149, 000, 147, 000, 148, 000, 000, 151, 000, 150, 129 };

	uint16_t i = 0, j = 0, s = 0;
	bool f_C3_seen = false;

	if(!s_file.startsWith("/")) s_file = "/" + s_file;
	while(s_file[i] != 0){                                     // convert UTF8 to ASCII
		if(s_file[i] == 195){                                   // C3
			i++;
			f_C3_seen = true;
			continue;
		}
		path[j] = s_file[i];
		if(path[j] > 128 && path[j] < 189 && f_C3_seen == true){
			s = ascii[path[j] - 129];
			if(s != 0) path[j] = s;                         // found a related ASCII sign
			f_C3_seen = false;
		}
		i++;
		j++;
	}
	path[j] = 0;

	File file;
	if(SD.exists(path)){
		file = SD.open(path); // #86
	}else{
		file = SD.open(s_file);
	}

	return file;
}

const ID3Metadata& SourceMP3mad::getMetadata() const{
	return metadata;
}

void SourceMP3mad::close(){
	mad_stream_finish(stream);
	mad_frame_finish(frame);
	mad_synth_finish(synth);

	free(stream);
	free(frame);
	free(synth);

	delete input;
	delete output;
}

int SourceMP3mad::available(){
	return file.available();
}

uint16_t SourceMP3mad::getDuration(){
	return 0;
}

uint16_t SourceMP3mad::getElapsed(){
	return 0;
}

void SourceMP3mad::seek(uint16_t time, fs::SeekMode mode){

}