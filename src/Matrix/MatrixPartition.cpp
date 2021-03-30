#include "MatrixPartition.h"

MatrixPartition::MatrixPartition(LEDmatrixImpl* matrix, uint8_t width, uint8_t height) : matrix(matrix), width(width), height(height){
	buffer = static_cast<uint8_t*>(malloc(width * height));
	pastBuffer = static_cast<uint8_t*>(malloc(width * height));
}

void MatrixPartition::drawPixel(int x, int y, uint8_t brightness){
	buffer[y * width + x] = brightness;
}

void MatrixPartition::clear(){
	memset(buffer, 0, width * height);
}

void MatrixPartition::vu(uint8_t amp){
	clear();
	uint8_t total = ((float) amp / 255.0f) * (float) height;
	for(int i = height-1; i >= height - total; i--){
		for(int j = 0; j < width; j++){
			drawPixel(j, i, 255);
		}
	}
}

void MatrixPartition::drawBitmap(int x, int y, uint width, uint height, uint8_t* data)
{
	for(int i = 0; i < height; i++)
	{
		for(int j = 0; j < width; j++)
		{
			drawPixel(x + j, y + i, data[i*width + j]);
		}
	}
}

void MatrixPartition::startAnimation(Animation* _animation, bool loop)
{
	stopAnimation();

	animation = _animation;
	animationLoop = loop;
	animationFrame = animation->getNextFrame();
	drawBitmap(0, 0, animation->getWidth(), animation->getHeight(), animationFrame->data);
	push();
	currentFrameTime = 0;
	animationStartMicros = micros();
}

/**************************************************************************/
/*!
	@brief  Stops running animation.
*/
/**************************************************************************/
void MatrixPartition::stopAnimation()
{
	delete animation;
	animation = nullptr;
	animationFrame = nullptr;
	currentFrameTime = 0;
}

/**************************************************************************/
/*!
	@brief  Updates running animation. Expected to be used inside loop().
	@param  _time Current millis() time, used for frame duration calculation.
*/
/**************************************************************************/
void MatrixPartition::loop(uint _time)
{
	if(animationFrame != nullptr && animation != nullptr){
		currentFrameTime+=_time;
		if(currentFrameTime >= animationFrame->duration*1000){
			clear();
			currentFrameTime = 0;
			animationFrame = animation->getNextFrame();
			if(animationFrame == nullptr){
				if(animationLoop){
					animationStartMicros = micros();
					animation->rewind();
					animationFrame = animation->getNextFrame();
				}else{
					stopAnimation();
					return;
				}
			}
			drawBitmap(0, 0, animation->getWidth(), animation->getHeight(), animationFrame->data);
			push();
			return;
		}
	}
}