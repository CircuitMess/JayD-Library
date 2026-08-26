#ifndef JAYD_LIBRARY_THREEBANDEQ_H
#define JAYD_LIBRARY_THREEBANDEQ_H

#include <Arduino.h>
#include "../Effect.h"

class ThreeBandEQ : public Effect {
public:
	enum class Band : uint8_t {
		Low,
		Mid,
		High,
		Count
	};

	static constexpr uint8_t NeutralLevel = 255;
	static constexpr uint8_t KillLevel = 0;

	ThreeBandEQ();

	void applyEffect(int16_t* inBuffer, int16_t* outBuffer, size_t numSamples) override;
	void setIntensity(uint8_t intensity) override;

	bool setLevel(Band band, uint8_t level);
	void reset();

private:
	struct Biquad {
		float b0 = 0.0f;
		float b1 = 0.0f;
		float b2 = 0.0f;
		float a1 = 0.0f;
		float a2 = 0.0f;
		float z1 = 0.0f;
		float z2 = 0.0f;

		float process(float input);
		void clear();
	};

	Biquad lowLowPass[2];
	Biquad lowHighPass[2];
	Biquad highLowPass[2];
	Biquad highHighPass[2];
	Biquad highAllPass;
	float gain[static_cast<uint8_t>(Band::Count)] = { 1.0f, 1.0f, 1.0f };

	static void configure(Biquad& filter, bool highPass, float frequency);
	static void configureAllPass(Biquad& filter, float frequency);
	static int16_t saturate(float sample);
};

#endif //JAYD_LIBRARY_THREEBANDEQ_H
