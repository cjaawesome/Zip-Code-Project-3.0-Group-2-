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

bool BlockBuffer::readRecordAtRBN(const uint32_t rbn, const uint32_t zipCode, const uint32_t blockSize, const size_t headerSize, ZipCodeRecord& outRecord)
{
    
   ActiveBlock block = loadActiveBlockAtRBN(rbn, blockSize, headerSize); //load block at rbn

   std::vector<ZipCodeRecord> records;
   recordBuffer.unpackBlock(block.data, records); //unpack block data into records

   auto it = std::find_if(records.begin(), records.end(), 
                          [zipCode](const ZipCodeRecord& rec) { return rec.getZipCode() == zipCode; });

    if (it != records.end()) 
    {
        outRecord = *it; // Record found
        return true;
    } 
    return false;
}

bool BlockBuffer::removeRecordAtRBN(const uint32_t rbn, const uint16_t minBlockSize, uint32_t& availListRBN, const uint32_t zipCode, const uint32_t blockSize, const size_t headerSize)
{
    ActiveBlock block = loadActiveBlockAtRBN(rbn, blockSize, headerSize); //load block at rbn

    std::vector<ZipCodeRecord> records;
    recordBuffer.unpackBlock(block.data, records); //unpack block data into records

    auto it = std::find_if(records.begin(), records.end(), 
                             [zipCode](const ZipCodeRecord& rec) { return rec.getZipCode() == zipCode; });
    if(it == records.end())
        return false; // Record not found

    records.erase(it); // Remove the record

    // Need to check min block size now. Return true if above minBlockSize. Merge blocks if below minimum size.
}

bool BlockBuffer::addRecord(const uint32_t rbn, const uint32_t blockSize, uint32_t& availListRBN, const ZipCodeRecord& record, const size_t headerSize)
{
    ActiveBlock block = loadActiveBlockAtRBN(rbn, blockSize, headerSize); //load block at rbn

    std::vector<ZipCodeRecord> records;
    recordBuffer.unpackBlock(block.data, records); //unpack block data into records

    uint32_t sizeOfRecords = 0;
    for (const auto& rec : records) 
    {
        sizeOfRecords += rec.getRecordSize();
    }
    
    if(sizeOfRecords + record.getRecordSize() <= blockSize) 
    {
        records.push_back(record); // Add the new record
        recordBuffer.packBlock(records, block.data); // Repack the block data
        block.recordCount = static_cast<uint16_t>(records.size()); // Update record count
        return writeActiveBlockAtRBN(rbn, block); // Write back to file
    } 
    // There was not enough room in the given record to add. Now need to try and merge or create new record.
    return false;
}

bool BlockBuffer::getMergeOccurred() const{
    return mergeOccurred;
}

bool BlockBuffer::writeActiveBlockAtRBN(const uint32_t rbn, const ActiveBlock& block){

}

bool BlockBuffer::tryJoinBlocks(const uint32_t rbn, uint32_t& availListRBN){
    
}

const std::string& BlockBuffer::getLastError() const{
    return lastError;
}

size_t BlockBuffer::getMemoryOffset(){
    return blockFile.tellg();
}

void BlockBuffer::readNextActiveBlock(ActiveBlock& block){

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

void BlockBuffer::setError(const std::string& message){
    errorState = true;//set error state to true
    lastError = message;//set error message
}

ActiveBlock BlockBuffer::loadActiveBlockAtRBN(const uint32_t rbn, const uint32_t blockSize, const size_t headerSize){
    ActiveBlock block;
    if (!blockFile.is_open()) { //if file can't open set error
        setError("file not open");
        return block;
    }

    std::streampos offset = headerSize + static_cast<std::streampos>(rbn) * blockSize;
    blockFile.seekg(offset);

    if (!blockFile.good()) { //if seek was out of file set error
        setError("failed to seek RBN number");
        return block;
    }

    block.succeedingRBN = rbn + 1;
    block.precedingRBN = rbn - 1;
    blockFile.read(block.data.data(), blockSize); //read block into block.data

    if (blockFile.gcount() == 0) { //if failed to read data set error
        setError("Failed to read block from file.");
        return block;
    }
    

    
    return block;
}

