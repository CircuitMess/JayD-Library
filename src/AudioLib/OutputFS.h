#ifndef JAYD_OUTPUTFS_H
#define JAYD_OUTPUTFS_H

#include "Output.h"
#include <FS.h>
#include <aacenc_lib.h>

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
	size_t dataLength;

	void writeHeaderWAV(size_t size);

	AACENC_BufDesc inBufDesc;
	AACENC_BufDesc outBufDesc;
	HANDLE_AACENCODER encoder;
	AACENC_InArgs inArgs;
	void setupBuffers();
};


#endif