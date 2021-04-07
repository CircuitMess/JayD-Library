#include "VuInfoGenerator.h"
#include "../AudioSetup.hpp"

void VuInfoGenerator::captureInfo(int16_t *outBuffer, size_t readBytes) {
	float EMA_a = 0.02;
	vu = abs(outBuffer[0]);
	for (int i = 1; i < readBytes; ++i) {
		vu = (EMA_a * abs(outBuffer[i])) + ((1 - EMA_a) * vu);
	}
	Serial.println(vu);
}

uint16_t VuInfoGenerator::getVu() {
	return vu;
}
