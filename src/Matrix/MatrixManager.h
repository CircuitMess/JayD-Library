#ifndef JAYD_LIBRARY_MATRIXMANAGER_H
#define JAYD_LIBRARY_MATRIXMANAGER_H

#include <Arduino.h>
#include "MatrixBig.h"
#include "MatrixL.h"
#include "MatrixR.h"

class MatrixManager : public LoopListener {
public:
	MatrixManager(LEDmatrixImpl* ledmatrix);
	MatrixBig matrixBig;
	MatrixL matrixL;
	MatrixR matrixR;

	void loop(uint time);
protected:
	LEDmatrixImpl* ledmatrix;
};


#endif //JAYD_LIBRARY_MATRIXMANAGER_H
