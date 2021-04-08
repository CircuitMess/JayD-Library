#include "MatrixMid.h"


MatrixMid::MatrixMid(LEDmatrixImpl* matrix) : MatrixPartition(matrix, 12, 2){ }

void MatrixMid::push(){
		uint8_t map[4] = { 136, 120, 104, 108};


	for(int i = 0; i < height; i++){
		for(int j = 0; j < width; j++){
			if(j > 7){
				matrix->drawPixel(map[i + 2] + j - 8, buffer[i * width + j]);
			}else{
				matrix->drawPixel(map[i] + j, buffer[i * width + j]);
			}
		}
	}
}

void MatrixMid::vu(uint8_t amp){
	clear();
	uint8_t total = ((float) amp / 255.0f) * (float) (width);
	Serial.println(total);
	for(int i = 0; i <= total+1; i++){
		for(int j = 0; j < height; j++){
			drawPixel(i, j, 255);
		}
	}
}