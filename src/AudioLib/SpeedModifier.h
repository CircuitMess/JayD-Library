#ifndef JAYD_LIBRARY_SPEEDMODIFIER_H
#define JAYD_LIBRARY_SPEEDMODIFIER_H

#include "Generator.h"
#include "Source.h"

#include <Buffer/DataBuffer.h>

class SpeedModifier : public Generator {

public:
	typedef uint32_t Rate;

	// Unsigned Q16.16 input-samples per output-sample. Setters clamp to
	// 0.5x-1.5x; positive halfway cases round up to the nearest Rate unit.
	static constexpr Rate RateScale = 1UL << 16;
	static constexpr Rate MinRate = RateScale / 2;
	static constexpr Rate NeutralRate = RateScale;
	static constexpr Rate MaxRate = RateScale + RateScale / 2;

	SpeedModifier(Source* source);
	~SpeedModifier() noexcept;

	size_t generate(int16_t* outBuffer) override;
	int available() override;

	/**
	 * Set the requested rate using the legacy 0-255 control. The two halves
	 * map linearly to 0.5x-1.0x and 1.0x-1.5x, with 127 exactly neutral.
	 */
	void setModifier(uint8_t modifier);

	/**
	 * Compatibility float setter. Prefer setRate() for exact control.
	 */
	void setSpeed(float speed);

	/**
	 * Set/get the requested Q16.16 rate. getCurrentRate() reports the
	 * click-reduction ramp position, which converges within 256 samples.
	 *
	 * This is resampling: playback pitch changes with rate. It does not
	 * provide key lock or time stretching.
	 */
	void setRate(Rate rate);
	Rate getRate() const;
	Rate getCurrentRate() const;
	void nudgeRate(int32_t amount);

	void setSource(Source* source);
	void reset();

private:
	Source *source = nullptr;
	DataBuffer* dataBuffer = nullptr;

	Rate requestedRate = NeutralRate;
	Rate currentRate = NeutralRate;
	uint32_t sourcePosition = 0;

	static Rate modifierToRate(uint8_t modifier);
	void advanceRate();
	void fillBuffer();
};


#endif //JAYD_LIBRARY_SPEEDMODIFIER_H
