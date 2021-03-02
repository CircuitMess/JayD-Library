#ifndef JAYD_OUTPUTFS_H
#define JAYD_OUTPUTFS_H

#include "Output.h"
#include <FS.h>


class OutputFS : public Output
{
public:
	OutputFS(const char* path, fs::FS* filesystem);
	~OutputFS();
	void start() override;
	void stop() override;

protected:
	void output(size_t numBytes) override;

private:
	const char* path;
	fs::FS* filesystem;
	fs::File* file;
	size_t recordingNum;
	size_t dataLength;

	void writeHeader();
};


#endif