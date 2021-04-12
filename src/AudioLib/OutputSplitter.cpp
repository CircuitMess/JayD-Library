#include "OutputSplitter.h"

OutputSplitter::OutputSplitter(){

}

void OutputSplitter::init(){

	for(auto output : outputs){
		output->init();
	}

}

void OutputSplitter::deinit(){

	for(auto output : outputs){
		output->deinit();
	}
}

void OutputSplitter::setOutput(size_t i, Output *output){

	free(output->inBuffer);
	output->inBuffer = nullptr;
	outputs[i] = output;
}

void OutputSplitter::addOutput(Output *newOutput){

	outputs.push_back(newOutput);
}

void OutputSplitter::output(size_t numSamples){

	for(auto output : outputs){

		output->inBuffer = inBuffer;
		output->output(numSamples);
		output->inBuffer = nullptr;
	}
}

Output* OutputSplitter::getOutput(size_t i){

	if(outputs.size() < i) return nullptr;

	return outputs.at(i);
}

void OutputSplitter::removeOutput(size_t i){

	if(outputs.empty()) return;
	outputs.erase(outputs.begin()+(i-1));
}
