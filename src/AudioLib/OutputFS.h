#ifndef JAYD_OUTPUTFS_H
#define JAYD_OUTPUTFS_H

#include "Output.h"
#include <FS.h>
#include <aacenc_lib.h>
#include "../Services/SDScheduler.h"

class OutputFS : public Output
{
public:
	OutputFS(const char* path, fs::FS* filesystem);
	~OutputFS();
	void init() override;
	void deinit() override;

protected:
	void output(size_t numBytes) override;

private:
	const char* path;
	fs::FS* filesystem;
	fs::File* file;
	size_t dataLength;

	void writeHeaderWAV(size_t size);

	AACENC_BufDesc inBufDesc;
	AACENC_BufDesc outBufDesc;
	HANDLE_AACENCODER encoder;
	AACENC_InArgs inArgs;
	void setupBuffers();

	SDResult* writeResult = nullptr;
	bool writePending = false;
	void addWriteJob(void* buffer, size_t size);
};


#endif