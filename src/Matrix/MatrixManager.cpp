#include "MatrixManager.h"

MatrixManager::MatrixManager(LEDmatrixImpl* ledmatrix) : ledmatrix(ledmatrix),
														matrixBig(ledmatrix),
														matrixL(ledmatrix),
														matrixR(ledmatrix){
}

void MatrixManager::loop(uint time){
	matrixR.loop(time);
	matrixL.loop(time);
	matrixBig.loop(time);
	ledmatrix->loop(time);
}