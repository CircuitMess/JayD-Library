#ifndef JAYD_LIBRARY_SPEEDMODIFIER_H
#define JAYD_LIBRARY_SPEEDMODIFIER_H

#include "Generator.h"
#include "Source.h"

#include <Buffer/DataBuffer.h>

class SpeedModifier : public Generator {

public:

	SpeedModifier(Source* source);
	~SpeedModifier();

	size_t generate(int16_t* outBuffer) override;
	int available() override;

	void setModifier(uint16_t _modifier);

private:

	Source *source = nullptr;
	DataBuffer* outBuff = nullptr;

	uint16_t modifier;
};


#endif //JAYD_LIBRARY_SPEEDMODIFIER_H
