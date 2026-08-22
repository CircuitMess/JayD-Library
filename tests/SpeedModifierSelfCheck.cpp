#include <AudioLib/SpeedModifier.h>

#include <cassert>
#include <climits>
#include <cstdint>

class FakeSource : public Source {
public:
	FakeSource(int16_t first, int16_t increment) : next(first), increment(increment){}

	size_t generate(int16_t* outBuffer) override{
		for(size_t i = 0; i < 256; i++){
			outBuffer[i] = next;
			next += increment;
		}
		return 256;
	}

	int available() override{
		return 4096;
	}

	uint16_t getDuration() override{
		return 1;
	}

	uint16_t getElapsed() override{
		return 0;
	}

	void seek(uint16_t, fs::SeekMode) override{}
	void close() override{}

private:
	int16_t next;
	int16_t increment;
};

int main(){
	int16_t output[256] = {};
	FakeSource ramp(0, 1);
	SpeedModifier speed(&ramp);

	assert(speed.getRate() == SpeedModifier::NeutralRate);
	assert(speed.getCurrentRate() == SpeedModifier::NeutralRate);
	assert(speed.generate(output) == 256);
	for(size_t i = 0; i < 256; i++) assert(output[i] == static_cast<int16_t>(i));

	speed.setRate(0);
	assert(speed.getRate() == SpeedModifier::MinRate);
	assert(speed.generate(output) == 256);
	assert(speed.getCurrentRate() == SpeedModifier::MinRate);

	speed.setRate(UINT32_MAX);
	assert(speed.getRate() == SpeedModifier::MaxRate);
	assert(speed.generate(output) == 256);
	assert(speed.getCurrentRate() == SpeedModifier::MaxRate);

	speed.setModifier(127);
	assert(speed.getRate() == SpeedModifier::NeutralRate);
	speed.setModifier(0);
	assert(speed.getRate() == SpeedModifier::MinRate);
	speed.setModifier(255);
	assert(speed.getRate() == SpeedModifier::MaxRate);
	speed.nudgeRate(INT32_MIN);
	assert(speed.getRate() == SpeedModifier::MinRate);
	speed.nudgeRate(INT32_MAX);
	assert(speed.getRate() == SpeedModifier::MaxRate);

	FakeSource first(100, 0);
	FakeSource replacement(200, 0);
	SpeedModifier swapped(&first);
	swapped.setRate(SpeedModifier::MaxRate);
	assert(swapped.generate(output) == 256);
	swapped.setSource(&replacement);
	assert(swapped.getRate() == SpeedModifier::MaxRate);
	assert(swapped.generate(output) == 256);
	for(size_t i = 0; i < 256; i++) assert(output[i] == 200);

	// MixSystem calls reset() after a channel seek to discard the stale
	// resampling position, but must not lose the DJ's chosen rate. This is
	// also the exact spot where cherry-picking the deck-rate rewrite (Q16.16
	// sourcePosition) together with the AAC-timing seek/reset glue silently
	// left reset() referencing the removed float `remainder` field.
	FakeSource seekSource(500, 1);
	SpeedModifier seeking(&seekSource);
	seeking.setRate(SpeedModifier::MaxRate);
	assert(seeking.generate(output) == 256);
	assert(seeking.getCurrentRate() == SpeedModifier::MaxRate);
	seeking.reset();
	assert(seeking.getRate() == SpeedModifier::MaxRate);
	assert(seeking.getCurrentRate() == SpeedModifier::MaxRate);
	assert(seeking.generate(output) == 256);
}
