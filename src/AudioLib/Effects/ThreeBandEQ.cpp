#include "ThreeBandEQ.h"
#include "../../AudioSetup.hpp"
#include <cmath>
#include <cstdint>

namespace {

// Hardware calibration points: tune these against the Jay-D output path.
constexpr float LowCrossoverHz = 250.0f;
constexpr float HighCrossoverHz = 3000.0f;
constexpr float ButterworthQ = 0.70710678f;

}

static_assert(SAMPLE_RATE == 24000, "ThreeBandEQ is calibrated for 24 kHz audio");
static_assert(NUM_CHANNELS == 1, "ThreeBandEQ is calibrated for mono audio");
static_assert(BUFFER_SAMPLES == 256, "ThreeBandEQ is calibrated for 256-sample blocks");
static_assert(BYTES_PER_SAMPLE == 2, "ThreeBandEQ requires 16-bit samples");

ThreeBandEQ::ThreeBandEQ(){
	for(uint8_t i = 0; i < 2; i++){
		configure(lowLowPass[i], false, LowCrossoverHz);
		configure(lowHighPass[i], true, LowCrossoverHz);
		configure(highLowPass[i], false, HighCrossoverHz);
		configure(highHighPass[i], true, HighCrossoverHz);
	}
	configureAllPass(highAllPass, HighCrossoverHz);
}

void ThreeBandEQ::applyEffect(int16_t* inBuffer, int16_t* outBuffer, size_t numSamples){
	for(size_t i = 0; i < numSamples; i++){
		const float input = inBuffer[i];
		float low = lowLowPass[1].process(lowLowPass[0].process(input));
		const float upper = lowHighPass[1].process(lowHighPass[0].process(input));
		const float mid = highLowPass[1].process(highLowPass[0].process(upper));
		const float high = highHighPass[1].process(highHighPass[0].process(upper));
		low = highAllPass.process(low);

		outBuffer[i] = saturate(low * gain[0] + mid * gain[1] + high * gain[2]);
	}
}

void ThreeBandEQ::setIntensity(uint8_t intensity){
	const uint8_t level = NeutralLevel - intensity;
	setLevel(Band::Low, level);
	setLevel(Band::Mid, level);
	setLevel(Band::High, level);
}

bool ThreeBandEQ::setLevel(Band band, uint8_t level){
	const uint8_t index = static_cast<uint8_t>(band);
	if(index >= static_cast<uint8_t>(Band::Count)) return false;

	gain[index] = static_cast<float>(level) / static_cast<float>(NeutralLevel);
	return true;
}

void ThreeBandEQ::reset(){
	for(uint8_t i = 0; i < 2; i++){
		lowLowPass[i].clear();
		lowHighPass[i].clear();
		highLowPass[i].clear();
		highHighPass[i].clear();
	}
	highAllPass.clear();
}

float ThreeBandEQ::Biquad::process(float input){
	const float output = b0 * input + z1;
	z1 = b1 * input - a1 * output + z2;
	z2 = b2 * input - a2 * output;
	return output;
}

void ThreeBandEQ::Biquad::clear(){
	z1 = 0.0f;
	z2 = 0.0f;
}

void ThreeBandEQ::configure(Biquad& filter, bool highPass, float frequency){
	const float omega = 2.0f * std::acos(-1.0f) * frequency / static_cast<float>(SAMPLE_RATE);
	const float cosine = std::cos(omega);
	const float alpha = std::sin(omega) / (2.0f * ButterworthQ);
	const float scale = 1.0f / (1.0f + alpha);
	const float direction = highPass ? 1.0f : -1.0f;

	filter.b0 = 0.5f * (1.0f + direction * cosine) * scale;
	filter.b1 = -(1.0f + direction * cosine) * scale;
	filter.b2 = filter.b0;
	if(!highPass) filter.b1 = -filter.b1;
	filter.a1 = -2.0f * cosine * scale;
	filter.a2 = (1.0f - alpha) * scale;
}

void ThreeBandEQ::configureAllPass(Biquad& filter, float frequency){
	Biquad prototype;
	configure(prototype, false, frequency);
	filter.b0 = prototype.a2;
	filter.b1 = prototype.a1;
	filter.b2 = 1.0f;
	filter.a1 = prototype.a1;
	filter.a2 = prototype.a2;
}

int16_t ThreeBandEQ::saturate(float sample){
	if(sample >= static_cast<float>(INT16_MAX)) return INT16_MAX;
	if(sample <= static_cast<float>(INT16_MIN)) return INT16_MIN;
	return static_cast<int16_t>(sample);
}
