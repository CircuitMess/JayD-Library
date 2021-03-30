#include "MatrixMid.h"


MatrixMid::MatrixMid(LEDmatrixImpl* matrix) : MatrixPartition(matrix, 12, 2){ }

void MatrixMid::push(){
		uint8_t map[4] = { 136, 120, 104, 108};


	for(int i = 0; i < height; i++){
		for(int j = 0; j < width; j++){
			if(j > 7){
				matrix->drawPixel(map[i + 2] + j - 8, buffer[(i+1) * width - j - 1]);
			}else{
				matrix->drawPixel(map[i] + j, buffer[(i+1) * width - j - 1]);
			}
		}
	}
}