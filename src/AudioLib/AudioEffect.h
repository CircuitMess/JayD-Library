#ifndef JAYD_AUDIOEFFECT_H
#define JAYD_AUDIOEFFECT_H

class AudioEffect
{
public:
	virtual ~AudioEffect() = default;
	virtual void applyEffect(int16_t *inBuffer, int16_t *outBuffer, int numSamples) = 0;
};

#endif