#include "BlockBuffer.h"
#include "RecordBuffer.h"



bool BlockBuffer::openFile(const std::string& filename, const size_t headerSize){
    blockFile.open(filename, std::ios::binary);
    if (!blockFile) { //if file couldn't open set error
        setError("Error opening file!");
        return false;
    }
    errorState = false;
    blockFile.seekg(headerSize); //skip header

    return true;
}

bool BlockBuffer::hasMoreData() const{
    return blockFile.is_open() && !blockFile.eof() && !errorState;
}

bool BlockBuffer::hasError() const{
    return errorState;
}

bool BlockBuffer::readRecordAtRBN(const uint32_t rbn, const uint32_t zipCode, ZipCodeRecord& outRecord){
    RecordBuffer buffer;
    blockFile.seekg(rbn); //seek random block number
}

bool BlockBuffer::removeRecordAtRBN(const uint32_t rbn, const uint32_t zipCode){

}

bool BlockBuffer::addRecord(const ZipCodeRecord& record){

}

bool BlockBuffer::getMergeOccurred() const{
    return mergeOccurred;
}

const std::string& BlockBuffer::getLastError() const{
    return lastError;
}

size_t BlockBuffer::getMemoryOffset(){
    return blockFile.tellg();
}

void BlockBuffer::readNextBlock(ActiveBlock& block){

}

void BlockBuffer::readNextAvailBlock(AvailBlock& block){

}

void BlockBuffer::closeFile(){
    blockFile.close(); // close the file
}

void BlockBuffer::dumpPhysicalOrder(std::ostream& out) const{

}

void BlockBuffer::dumpLogicalOrder(std::ostream& out) const{

}

uint32_t BlockBuffer::getRecordsProcessed() const{

}

uint32_t BlockBuffer::getBlocksProcessed() const{

}

bool BlockBuffer::tryJoinBlocks(ActiveBlock& block1, ActiveBlock& block2){

}

void BlockBuffer::setError(const std::string& message){
    errorState = true;//set error state to true
    lastError = message;//set error message
}

ActiveBlock BlockBuffer::loadActiveBlockAtRBN(const uint32_t rbn){
    
}

bool BlockBuffer::writeActiveBlockAtRBN(const uint32_t rbn, const ActiveBlock& block){

}