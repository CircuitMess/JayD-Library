#include <SD.h>
#include "MixSystem.h"
#include "../../JayD.h"
#include "../Effects/LowPass.h"
#include "../Effects/HighPass.h"
#include "../Effects/Reverb.h"
#include "../Effects/BitCrusher.h"
#include "../../Settings.h"
#include "../../PerfMon.h"

MixSystem::MixSystem(const fs::File& f1, const fs::File& f2) : MixSystem(){
	open(0, f1);
	open(1, f2);
}

MixSystem::MixSystem() : audioTask("MixAudio", audioThread, 16 * 1024, this), queue(requestCapacity, sizeof(uint8_t)){
	mixer = new Mixer();

	for(int i = 0; i < 2; i++){
		effector[i] = new EffectProcessor(source[i]);

		for(int j = 0; j < 3; j++){
			effector[i]->addEffect(nullptr);
		}

		mixer->addSource(effector[i]);
	}

	i2s = new OutputI2S({
								.mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX),
								.sample_rate = SAMPLE_RATE,
								.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
								.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
								.communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
								.intr_alloc_flags = 0,
								.dma_buf_count = 16,
								.dma_buf_len = 512,
								.use_apll = false
						}, i2s_pin_config, I2S_NUM_0);

	i2s->setGain(0.4f*((float) Settings.get().volumeLevel) / 255.0f);
	i2s->setSource(mixer);

	fsOut = new OutputWAV();

	out = new OutputSplitter();
	out->addOutput(i2s);
	out->setSource(mixer);
}

MixSystem::~MixSystem(){
	stop();
	Sched.loop(0);
	delete out;
	delete fsOut;
	delete i2s;
	delete mixer;
	for(int i = 0; i < 2; i++){
		for(int j = 0; j < 3; j++){
			delete effector[i]->getEffect(j);
		}

		delete effector[i];
		delete speed[i];

		delete source[i];
		delete retiredSource[i];
	}
}

bool MixSystem::open(uint8_t c, const fs::File& file){
	if(c >= 2 || !file){
		Serial.println("MixSystem: file not open");
		return false;
	}

	auto newSource = new SourceAAC(file);
	if(newSource == nullptr){
		Serial.println("MixSystem: source allocation failed");
		return false;
	}
	newSource->setRepeat(true);
	while(!newSource->isReadReady()) Sched.loop(0);
	return replaceSource(c, newSource);
}

bool MixSystem::replaceSource(uint8_t c, SourceAAC* newSource){
	if(c >= 2 || newSource == nullptr){
		delete newSource;
		return false;
	}

	sourceMutex.lock();
	if(retiredSource[c] != nullptr){
		sourceMutex.unlock();
		delete newSource;
		return false;
	}
	newSource->setVolume(volume[c]);
	auto oldSource = source[c];
	source[c] = newSource;
	if(speed[c]){
		speed[c]->setSource(newSource);
	}else{
		effector[c]->setSource(newSource);
	}
	retiredSource[c] = oldSource;
	sourceMutex.unlock();

	return true;
}

bool MixSystem::openChannel(uint8_t channel, const fs::File& file){
	if(channel >= 2 || !file) return false;

	if(!out->isRunning()){
		return open(channel, file);
	}

	cleanupRetiredSources();
	sourceMutex.lock();
	const bool canReplace = retiredSource[channel] == nullptr;
	sourceMutex.unlock();
	if(!canReplace) return false;

	const int8_t requestIndex = reserveRequest({ MixRequest::OPEN, channel });
	if(requestIndex < 0) return false;

	auto newSource = new SourceAAC(file);
	if(newSource == nullptr){
		releaseRequest(requestIndex);
		return false;
	}
	newSource->setRepeat(true);
	while(!newSource->isReadReady()) Sched.loop(0);
	requests[requestIndex].value = reinterpret_cast<size_t>(newSource);

	if(sendRequest(requestIndex)) return true;

	delete newSource;
	releaseRequest(requestIndex);
	return false;
}

void MixSystem::_openChannel(uint8_t channel, SourceAAC* newSource){
	if(channel >= 2 || newSource == nullptr){
		delete newSource;
		return;
	}

	const bool wasPaused = mixer->isChannelPaused(channel);
	mixer->pauseChannel(channel);
	if(replaceSource(channel, newSource) || !wasPaused) mixer->resumeChannel(channel);
}

int8_t MixSystem::reserveRequest(const MixRequest& request){
	queueMutex.lock();
	for(uint8_t i = 0; i < requestCapacity; i++){
		if(requestUsed[i] && request.type == MixRequest::OPEN &&
		   requests[i].type == MixRequest::OPEN && requests[i].channel == request.channel){
			queueMutex.unlock();
			return -1;
		}
	}

	for(uint8_t i = 0; i < requestCapacity; i++){
		if(requestUsed[i]) continue;
		requests[i] = request;
		requestUsed[i] = true;
		queueMutex.unlock();
		return i;
	}
	queueMutex.unlock();
	return -1;
}

bool MixSystem::sendRequest(uint8_t index){
	if(index >= requestCapacity || !requestUsed[index]) return false;
	return queue.send(&index);
}

bool MixSystem::enqueueRequest(const MixRequest& request){
	const int8_t index = reserveRequest(request);
	if(index < 0) return false;
	if(sendRequest(index)) return true;
	releaseRequest(index);
	return false;
}

void MixSystem::releaseRequest(uint8_t index){
	if(index >= requestCapacity) return;
	queueMutex.lock();
	requests[index] = {};
	requestUsed[index] = false;
	queueMutex.unlock();
}

void MixSystem::clearRequests(){
	uint8_t index;
	while(queue.count()){
		if(!queue.receive(&index)) break;
		if(index < requestCapacity && requestUsed[index] && requests[index].type == MixRequest::OPEN){
			delete reinterpret_cast<SourceAAC*>(requests[index].value);
		}
		releaseRequest(index);
	}
}

void MixSystem::cleanupRetiredSources(){
	SourceAAC* ready[2] = {};
	sourceMutex.lock();
	for(uint8_t channel = 0; channel < 2; channel++){
		if(retiredSource[channel] && retiredSource[channel]->isReadReady()){
			ready[channel] = retiredSource[channel];
			retiredSource[channel] = nullptr;
		}
	}
	sourceMutex.unlock();
	delete ready[0];
	delete ready[1];
}

void MixSystem::audioThread(Task* task){
	MixSystem* system = static_cast<MixSystem*>(task->arg);

	Serial.println("-- MixSystem started --");

	while(task->running){
		uint8_t requestIndex;
		while(system->queue.count()){
			if(!system->queue.receive(&requestIndex)) break;
			if(requestIndex >= requestCapacity || !system->requestUsed[requestIndex]) continue;
			const MixRequest request = system->requests[requestIndex];

			switch(request.type){
				case MixRequest::ADD_SPEED:
					system->_addSpeed(request.channel);
					break;
				case MixRequest::REMOVE_SPEED:
					system->_removeSpeed(request.channel);
					break;
				case MixRequest::SET_SPEED:
					system->_setSpeed(request.channel, request.value);
					break;
				case MixRequest::SET_EFFECT:
					system->_setEffect(request.channel, request.slot, static_cast<EffectType>(request.value));
					break;
				case MixRequest::SET_EFFECT_INTENSITY:
					system->_setEffectIntensity(request.channel, request.slot, request.value);
					break;
				case MixRequest::SET_INFO:
					system->_setInfoGenerator(request.channel, reinterpret_cast<InfoGenerator*>(request.value));
					break;
				case MixRequest::SET_SEEK:
					system->_seekChannel(request.channel, static_cast<uint16_t>(request.value));
					break;
				case MixRequest::RECORD:
					if(request.value == system->isRecording()) break;
					if(request.value){
						system->_startRecording();
					}else{
						system->_stopRecording();
					}
					break;
				case MixRequest::OPEN:
					system->_openChannel(request.channel, reinterpret_cast<SourceAAC*>(request.value));
					break;
			}
			system->releaseRequest(requestIndex);
		}

		if(system->out->isRunning()){
			Profiler.init();
			system->out->loop(0);
			Profiler.report();
		}else{
			system->running = false;
		}
	}

	system->fsOut->stop();
}

void MixSystem::start(){
	if(running) return;

	running = true;
	out->start();
	audioTask.start(1, 0);
}

void MixSystem::stop(){
	if(!audioTask.isStopped()){
		audioTask.stop();
		while(!audioTask.isStopped()){
			Sched.loop(0);
		}
	}

	running = false;
	_stopRecording();
	fileOut.close();

	out->stop();
	clearRequests();
}

bool MixSystem::isRunning(){
	return running;
}

uint16_t MixSystem::getDuration(uint8_t c){
	if(c >= 2) return 0;
	cleanupRetiredSources();
	sourceMutex.lock();
	uint16_t duration = source[c] ? source[c]->getDuration() : 0;
	sourceMutex.unlock();
	return duration;
}

uint16_t MixSystem::getElapsed(uint8_t c){
	if(c >= 2) return 0;
	cleanupRetiredSources();
	if(seekPending[c] > 0){
		return seek[c];
	}
	sourceMutex.lock();
	uint16_t elapsed = source[c] ? source[c]->getElapsed() : 0;
	sourceMutex.unlock();
	return elapsed;
}

bool MixSystem::hasChannel(uint8_t c){
	if(c >= 2) return false;
	cleanupRetiredSources();
	sourceMutex.lock();
	bool loaded = source[c] != nullptr;
	sourceMutex.unlock();
	return loaded;
}

uint8_t MixSystem::getVolume(uint8_t c){
	return c < 2 ? volume[c] : 0;
}

uint8_t MixSystem::getMix(){
	return mixer ? mixer->getMixRatio() : 128;
}

void MixSystem::setVolume(uint8_t c, uint8_t volume){
	if(c >= 2) return;
	this->volume[c] = volume;
	sourceMutex.lock();
	if(source[c]) source[c]->setVolume(volume);
	sourceMutex.unlock();
}

void MixSystem::setMix(uint8_t ratio){
	if(!mixer) return;
	mixer->setMixRatio(ratio);
}

void MixSystem::addSpeed(uint8_t channel){
	if(!out->isRunning()){
		_addSpeed(channel);
		return;
	}

	enqueueRequest({ MixRequest::ADD_SPEED, channel });
}

void MixSystem::removeSpeed(uint8_t channel){
	if(!out->isRunning()){
		_removeSpeed(channel);
		return;
	}

	enqueueRequest({ MixRequest::REMOVE_SPEED, channel });
}

void MixSystem::setSpeed(uint8_t channel, uint8_t speed){
	if(!out->isRunning()){
		_setSpeed(channel, speed);
		return;
	}

	enqueueRequest({ MixRequest::SET_SPEED, channel, 0, speed });
}

void MixSystem::setEffect(uint8_t channel, uint8_t slot, EffectType type){
	if(!out->isRunning()){
		_setEffect(channel, slot, type);
		return;
	}

	enqueueRequest({ MixRequest::SET_EFFECT, channel, slot, static_cast<uint8_t>(type) });
}

void MixSystem::setEffectIntensity(uint8_t channel, uint8_t slot, uint8_t intensity){
	if(!out->isRunning()){
		_setEffectIntensity(channel, slot, intensity);
		return;
	}

	enqueueRequest({ MixRequest::SET_EFFECT_INTENSITY, channel, slot, intensity });
}

void MixSystem::_addSpeed(uint8_t c){
	if(c >= 2 || !effector[c] || !source[c] || speed[c]) return;
	auto speed = this->speed[c] = new SpeedModifier(source[c]);
	effector[c]->setSource(speed);
}

void MixSystem::_removeSpeed(uint8_t c){
	if(c >= 2 || !effector[c] || !speed[c]) return;
	effector[c]->setSource(source[c]);
	delete speed[c];
	speed[c] = nullptr;
}

void MixSystem::_setSpeed(uint8_t c, uint8_t modifier){
	if(c >= 2 || !this->speed[c]) return;
	this->speed[c]->setModifier(modifier);
}

void MixSystem::_setEffect(uint8_t c, uint8_t s, EffectType type){
	if(c >= 2 || s >= 3 || !effector[c]) return;
	delete effector[c]->getEffect(s);
	effector[c]->setEffect(s, getEffect[type]());
}

void MixSystem::_setEffectIntensity(uint8_t c, uint8_t s, uint8_t intensity){
	if(c >= 2 || s >= 3 || !effector[c] || !effector[c]->getEffect(s)) return;
	effector[c]->getEffect(s)->setIntensity(intensity);
}

Effect* (* MixSystem::getEffect[])() = {
		[]() -> Effect*{ return nullptr; }, // None
		[]() -> Effect*{ return nullptr; }, // Speed
		[]() -> Effect*{ return new LowPass(); },
		[]() -> Effect*{ return new HighPass(); },
		[]() -> Effect*{ return new Reverb(); },
		[]() -> Effect*{ return new BitCrusher(); }
};

void MixSystem::setOutInfo(InfoGenerator* outInfoGen){
	setChannelInfo(2, outInfoGen);
}

void MixSystem::_setInfoGenerator(uint8_t channel, InfoGenerator* generator){
	if(channel > 2 || generator == nullptr) return;
	if(channel == 2){
		out->setSource(generator);
		generator->setSource(mixer);
	}else{
		mixer->setSource(channel, generator);
		generator->setSource(effector[channel]);
	}
}

void MixSystem::setChannelInfo(uint8_t channel, InfoGenerator* channelInfoGen){
	if(!out->isRunning()){
		_setInfoGenerator(channel, channelInfoGen);
		return;
	}

	enqueueRequest({ MixRequest::SET_INFO, channel, 0, reinterpret_cast<size_t>(channelInfoGen) });
}

void MixSystem::pauseChannel(uint8_t channel){
	mixer->pauseChannel(channel);
}

void MixSystem::resumeChannel(uint8_t channel){
	mixer->resumeChannel(channel);
	if(!out->isRunning()){
		out->start();
	}
}

void MixSystem::seekChannel(uint8_t channel, uint16_t time){
	if(channel >= 2) return;

	if(!out->isRunning()){
		_seekChannel(channel, time);
		return;
	}

	seek[channel] = time;
	seekPending[channel]++;
	if(!enqueueRequest({ MixRequest::SET_SEEK, channel, 0, time })) seekPending[channel]--;
}

void MixSystem::_seekChannel(uint8_t channel, uint16_t time){
	if(channel > 1) return;
	if(seekPending[channel] > 0) seekPending[channel]--;
	if(!source[channel]) return;

	source[channel]->seek(time, SeekSet);
}

bool MixSystem::isRecording(){
	return out->getOutput(1) != nullptr;
}

void MixSystem::startRecording(){
	if(!out->isRunning()){
		_startRecording();
		return;
	}

	enqueueRequest({ MixRequest::RECORD, 0, 0, 1 });
}

void MixSystem::stopRecording(){
	if(!out->isRunning()){
		_stopRecording();
		return;
	}

	enqueueRequest({ MixRequest::RECORD, 0, 0, 0 });
}

void MixSystem::_startRecording(){
	if(isRecording()) return;

	if(SD.exists(recordPath)){
		SD.remove(recordPath);
	}

	fileOut = SD.open(recordPath, "w");
	if(!fileOut){
		Serial.printf("Failed opening %s for writing\n", recordPath);
		return;
	}

	fsOut->setFile(fileOut);

	out->addOutput(fsOut);

	if(out->isRunning()){
		fsOut->start();
	}
}

void MixSystem::_stopRecording(){
	if(!isRecording()) return;

	out->removeOutput(1);

	fsOut->stop();
	fileOut.close();
}

bool MixSystem::isChannelPaused(uint8_t channel){
	if(!mixer) return false;
	return mixer->isChannelPaused(channel);
}

void MixSystem::setChannelDoneCallback(uint8_t channel, void(*callback)()) {
	if(channel >= 2) return;
	sourceMutex.lock();
	if(source[channel]) source[channel]->setSongDoneCallback(callback);
	sourceMutex.unlock();
}
