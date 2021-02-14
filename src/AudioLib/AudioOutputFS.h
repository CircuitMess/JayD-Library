#ifndef JAYD_AUDIOOUTPUTFS_H
#define JAYD_AUDIOOUTPUTFS_H

#include "AudioOutput.h"
#include <FS.h>


class AudioOutputFS : public AudioOutput
{
public:
	AudioOutputFS(const char* path, fs::FS* filesystem);
	~AudioOutputFS();

protected:
	void output(size_t numBytes) override;
	void start() override;
	void stop() override;

private:
	const char* path;
	fs::FS* filesystem;
	fs::File* file;
	size_t recordingNum;
	size_t dataLength;

	void writeHeader();
};


#endif