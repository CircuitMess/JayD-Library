#include "MatrixManager.h"

MatrixManager::MatrixManager(LEDmatrixImpl* ledmatrix) : ledmatrix(ledmatrix),
														matrixBig(ledmatrix),
														matrixL(ledmatrix),
														matrixR(ledmatrix),
														matrixMid(ledmatrix){
}

void MatrixManager::loop(uint time){
	matrixR.loop(time);
	matrixL.loop(time);
	matrixBig.loop(time);
	matrixMid.loop(time);
	ledmatrix->loop(time);
}

void MatrixManager::push(){
	matrixR.push();
	matrixL.push();
	matrixBig.push();
	matrixMid.push();
}

void MatrixManager::stopAnimation(){
	matrixR.stopAnimation();
	matrixL.stopAnimation();
	matrixBig.stopAnimation();
	matrixMid.stopAnimation();
}

void MatrixManager::clear(){
	matrixR.clear();
	matrixL.clear();
	matrixBig.clear();
	matrixMid.clear();
	ledmatrix->clear();
}