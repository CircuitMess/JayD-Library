#ifndef JAYD_LIBRARY_MIXSYSTEM_H
#define JAYD_LIBRARY_MIXSYSTEM_H


#include <Util/Task.h>
#include "../OutputI2S.h"
#include "../SpeedModifier.h"
#include "../EffectProcessor.h"
#include "../Mixer.h"
#include "../SourceWAV.h"
#include "../EffectType.hpp"
#include "../SourceAAC.h"
#include <Sync/Queue.h>
#include "../InfoGenerator.h"

struct MixRequest {
	enum { ADD_SPEED, REMOVE_SPEED, SET_SPEED, SET_EFFECT, SET_EFFECT_INTENSITY, SET_INFO, SET_SEEK } type;
	uint8_t channel;
	uint8_t slot;
	size_t value;
};

class MixSystem {
public:
	MixSystem();
	MixSystem(const fs::File& f1, const fs::File& f2);
	virtual ~MixSystem();

	bool open(uint8_t channel, const fs::File& file);

	Task audioTask;
	static void audioThread(Task* task);

	void start();
	void stop();

	uint16_t getDuration(uint8_t channel);
	uint16_t getElapsed(uint8_t channel);

	void setVolume(uint8_t channel, uint8_t volume);
	void setMix(uint8_t ratio);

	void addSpeed(uint8_t channel);
	void removeSpeed(uint8_t channel);
	void setSpeed(uint8_t channel, uint8_t speed);
	void setEffect(uint8_t channel, uint8_t slot, EffectType type);
	void setEffectIntensity(uint8_t channel, uint8_t slot, uint8_t intensity);

	void setOutInfo(InfoGenerator* outInfoGen);
	void setChannelInfo(uint8_t channel, InfoGenerator* channelInfoGen);

	void pauseChannel(uint8_t channel);
	void resumeChannel(uint8_t channel);

	void seekChannel(uint8_t channel, uint16_t time);

private:
	bool running = false;
	Queue queue;

	fs::File file[2];

	SourceAAC* source[2] = { nullptr };

	EffectProcessor* effector[2];
	Mixer* mixer;
	OutputI2S* out;

	SpeedModifier* speed[2] = { nullptr };

	void _addSpeed(uint8_t channel);
	void _removeSpeed(uint8_t channel);
	void _setSpeed(uint8_t channel, uint8_t speed);
	void _setEffect(uint8_t channel, uint8_t slot, EffectType type);
	void _setEffectIntensity(uint8_t channel, uint8_t slot, uint8_t intensity);
	void _setInfoGenerator(uint8_t channel, InfoGenerator* generator);
	void _seekChannel(uint8_t channel, uint16_t time);

	static Effect* (* getEffect[EffectType::COUNT])();

};

#endif //JAYD_LIBRARY_MIXSYSTEM_H
