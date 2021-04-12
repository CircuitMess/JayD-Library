#ifndef JAYD_LIBRARY_OUTPUTSPLITTER_H
#define JAYD_LIBRARY_OUTPUTSPLITTER_H

#include <vector>
#include "Output.h"

class OutputSplitter : public Output {

public:

	OutputSplitter();

	void addOutput(Output* newOutput);
	void setOutput(size_t i, Output* output);
	Output* getOutput(size_t i);
	void removeOutput(size_t i);


protected:

	void output(size_t numSamples) override;

	void init() override;
	void deinit() override;

private:

	std::vector<Output*> outputs;

};


#endif //JAYD_LIBRARY_OUTPUTSPLITTER_H
