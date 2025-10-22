#include "BlockBuffer.h"



bool BlockBuffer::openFile(const std::string& filename, const size_t headerSize){
    blockFile.open(filename, std::ios::binary);
    if (!blockFile) { //if file couldn't open set error
        setError("Error opening file!");
        errorState = true;
        return false;
    }
    blockFile.seekg(headerSize); //skip header

    return true;
}

bool BlockBuffer::hasMoreData() const{
    return blockFile.is_open() && !blockFile.eof() && !errorState;
}

