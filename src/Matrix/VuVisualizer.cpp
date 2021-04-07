#include "VuVisualizer.h"

VuVisualizer::VuVisualizer(MatrixPartition *partition) : Visualizer(partition) {

}

InfoGenerator *VuVisualizer::getInfoGenerator() {
	return &info;
}

void VuVisualizer::loop(uint _millis) {
	matrix->vu(info.getVu());
	matrix->push();
}

