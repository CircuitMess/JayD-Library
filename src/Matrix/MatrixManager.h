#ifndef JAYD_LIBRARY_MATRIXMANAGER_H
#define JAYD_LIBRARY_MATRIXMANAGER_H

#include <Arduino.h>
#include "MatrixBig.h"
#include "MatrixL.h"
#include "MatrixR.h"
#include "MatrixMid.h"

class MatrixManager : public LoopListener {
public:
	MatrixManager(LEDmatrixImpl* ledmatrix);
	MatrixBig matrixBig;
	MatrixL matrixL;
	MatrixR matrixR;
	MatrixMid matrixMid;

	void loop(uint time);
	void push();
	void stopAnimation();
	void clear();
protected:
	LEDmatrixImpl* ledmatrix;
};


#endif //JAYD_LIBRARY_MATRIXMANAGER_H
