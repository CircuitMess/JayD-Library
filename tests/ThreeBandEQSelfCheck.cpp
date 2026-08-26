#include "AudioLib/Effects/ThreeBandEQ.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

namespace {

constexpr double Pi = 3.14159265358979323846;
constexpr double SampleRate = 24000.0;
constexpr size_t BlockSamples = 256;
constexpr size_t WarmupSamples = 12000;
constexpr size_t MeasuredSamples = 24000;

struct Levels {
	uint8_t low;
	uint8_t mid;
	uint8_t high;
};

double currentResponse(ThreeBandEQ& eq, double frequency){
	std::array<int16_t, BlockSamples> input = {};
	std::array<int16_t, BlockSamples> output = {};
	const size_t totalSamples = WarmupSamples + MeasuredSamples;
	double inputPower = 0.0;
	double outputPower = 0.0;

	eq.reset();

	for(size_t offset = 0; offset < totalSamples; offset += BlockSamples){
		const size_t count = std::min(BlockSamples, totalSamples - offset);
		for(size_t i = 0; i < count; i++){
			const double phase = 2.0 * Pi * frequency * static_cast<double>(offset + i) / SampleRate;
			input[i] = static_cast<int16_t>(12000.0 * std::sin(phase));
		}
		eq.applyEffect(input.data(), output.data(), count);

		for(size_t i = 0; i < count; i++){
			if(offset + i < WarmupSamples) continue;
			inputPower += static_cast<double>(input[i]) * input[i];
			outputPower += static_cast<double>(output[i]) * output[i];
		}
	}

	return std::sqrt(outputPower / inputPower);
}

double response(ThreeBandEQ& eq, double frequency, Levels levels){
	assert(eq.setLevel(ThreeBandEQ::Band::Low, levels.low));
	assert(eq.setLevel(ThreeBandEQ::Band::Mid, levels.mid));
	assert(eq.setLevel(ThreeBandEQ::Band::High, levels.high));
	return currentResponse(eq, frequency);
}

void checkNeutralAndKills(){
	ThreeBandEQ eq;
	const std::array<double, 3> frequencies = {{ 60.0, 1000.0, 8000.0 }};
	const std::array<ThreeBandEQ::Band, 3> bands = {{
			ThreeBandEQ::Band::Low,
			ThreeBandEQ::Band::Mid,
			ThreeBandEQ::Band::High
	}};
	const Levels neutral = { 255, 255, 255 };

	for(double frequency : frequencies){
		const double neutralGain = response(eq, frequency, neutral);
		assert(std::abs(20.0 * std::log10(neutralGain)) <= 0.1);
	}

	for(size_t selected = 0; selected < bands.size(); selected++){
		Levels isolated = { 0, 0, 0 };
		Levels killed = neutral;
		if(selected == 0){
			isolated.low = 255;
			killed.low = 0;
		}else if(selected == 1){
			isolated.mid = 255;
			killed.mid = 0;
		}else{
			isolated.high = 255;
			killed.high = 0;
		}

		const double selectedGain = response(eq, frequencies[selected], isolated);
		const double killedGain = response(eq, frequencies[selected], killed);
		assert(selectedGain >= 0.95);
		assert(killedGain <= 0.02);

		for(size_t neighbor = 0; neighbor < bands.size(); neighbor++){
			if(neighbor == selected) continue;
			Levels neighborOnly = { 0, 0, 0 };
			if(neighbor == 0) neighborOnly.low = 255;
			if(neighbor == 1) neighborOnly.mid = 255;
			if(neighbor == 2) neighborOnly.high = 255;
			assert(selectedGain >= 20.0 * response(eq, frequencies[selected], neighborOnly));
		}
	}
}

void checkGainCorners(){
	ThreeBandEQ eq;
	const std::array<double, 3> frequencies = {{ 60.0, 1000.0, 8000.0 }};
	for(uint8_t mask = 0; mask < 8; mask++){
		const Levels levels = {
				static_cast<uint8_t>((mask & 1) ? 255 : 0),
				static_cast<uint8_t>((mask & 2) ? 255 : 0),
				static_cast<uint8_t>((mask & 4) ? 255 : 0)
		};
		for(double frequency : frequencies){
			assert(response(eq, frequency, levels) <= 1.001);
		}
	}

	assert(!eq.setLevel(static_cast<ThreeBandEQ::Band>(255), 255));

	std::array<int16_t, BlockSamples> input;
	std::array<int16_t, BlockSamples> output;
	input.fill(INT16_MAX);
	eq.setIntensity(255);
	eq.applyEffect(input.data(), output.data(), output.size());
	for(int16_t sample : output) assert(sample == 0);
}

void checkReset(){
	ThreeBandEQ eq;
	std::array<int16_t, BlockSamples> input = {};
	std::array<int16_t, BlockSamples> output = {};
	input[0] = INT16_MAX;

	assert(eq.setLevel(ThreeBandEQ::Band::Low, 0));
	assert(eq.setLevel(ThreeBandEQ::Band::Mid, 255));
	assert(eq.setLevel(ThreeBandEQ::Band::High, 255));
	eq.applyEffect(input.data(), output.data(), output.size());

	eq.reset();
	input.fill(0);
	eq.applyEffect(input.data(), output.data(), output.size());
	for(int16_t sample : output) assert(sample == 0);

	const double retainedKill = currentResponse(eq, 60.0);
	assert(retainedKill <= 0.02);
}

}

int main(){
	checkNeutralAndKills();
	checkGainCorners();
	checkReset();
	std::cout << "ThreeBandEQ self-check passed\n";
	return 0;
}
