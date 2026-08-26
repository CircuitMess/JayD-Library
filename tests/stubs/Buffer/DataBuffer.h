#ifndef DATABUFFER_H
#define DATABUFFER_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

class DataBuffer {
public:
	DataBuffer(size_t size, bool = false) : buffer(size){}

	size_t readAvailable(){
		return writeCursor - readCursor;
	}

	bool readMove(size_t amount){
		if(readCursor + amount > writeCursor) return false;
		readCursor += amount;
		return true;
	}

	const uint8_t* readData(){
		return buffer.data() + readCursor;
	}

	size_t writeAvailable(){
		return readCursor + buffer.size() - writeCursor;
	}

	bool writeMove(size_t amount){
		if(writeCursor + amount > buffer.size()) return false;
		writeCursor += amount;
		return true;
	}

	uint8_t* writeData(){
		const size_t left = readAvailable();
		if(readCursor != 0 && left != 0) std::memmove(buffer.data(), buffer.data() + readCursor, left);
		readCursor = 0;
		writeCursor = left;
		return buffer.data() + writeCursor;
	}

	void clear(){
		readCursor = 0;
		writeCursor = 0;
	}

private:
	std::vector<uint8_t> buffer;
	size_t readCursor = 0;
	size_t writeCursor = 0;
};

#endif
