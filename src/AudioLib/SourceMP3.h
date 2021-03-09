#ifndef JAYD_SOURCEMP3_H
#define JAYD_SOURCEMP3_H

#include <Arduino.h>
#include <FS.h>
#include "Source.h"

class SourceMP3 : public Source
{
public:
	SourceMP3();
	SourceMP3(fs::File *file);
	~SourceMP3();
	size_t generate(int16_t* outBuffer) override;
	int available() override;

	void open(fs::File *file);

private:
	fs::File *file = nullptr;

};


#endif