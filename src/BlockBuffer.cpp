#include "BlockBuffer.h"
#include "RecordBuffer.h"



bool BlockBuffer::openFile(const std::string& filename, const size_t headerSize){
    blockFile.open(filename, std::ios::binary | std::ios::in | std::ios::out);
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
        return writeActiveBlockAtRBN(rbn, blockSize, headerSize, block); // Write back to file
    } 
    // There was not enough room in the given record to add. Now need to try and merge or create new record.
    return false;
}

bool BlockBuffer::getMergeOccurred() const{
    return mergeOccurred;
}

bool BlockBuffer::writeActiveBlockAtRBN(const uint32_t rbn, const uint32_t blockSize, const size_t headerSize, const ActiveBlock& block)
{
    if (!blockFile.is_open()) 
    {
        setError("file not open");
        return false;
    }

    // Calculate position
    std::streampos offset = headerSize + static_cast<std::streampos>(rbn) * blockSize;
    blockFile.seekp(offset);

    if (!blockFile.good()) 
    {
        setError("failed to seek to RBN");
        return false;
    }

    // Write metadata
    blockFile.write(reinterpret_cast<const char*>(&block.recordCount), sizeof(uint16_t));
    blockFile.write(reinterpret_cast<const char*>(&block.precedingRBN), sizeof(uint32_t));
    blockFile.write(reinterpret_cast<const char*>(&block.succeedingRBN), sizeof(uint32_t));

    // Write block data
    blockFile.write(block.data.data(), block.data.size());

    // Padding
    size_t bytesWritten = 10 + block.data.size(); // 10 = metadata size
    if (bytesWritten < blockSize) {
        std::vector<char> padding(blockSize - bytesWritten, 0);
        blockFile.write(padding.data(), padding.size());
    }

    return blockFile.good();
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
    if (!blockFile.is_open()) 
    {
        setError("file not open");
        return block;
    }

    std::streampos offset = headerSize + static_cast<std::streampos>(rbn) * blockSize;
    blockFile.seekg(offset);

    if (!blockFile.good()) 
    {
        setError("failed to seek RBN number");
        return block;
    }

    // Read metadata
    blockFile.read(reinterpret_cast<char*>(&block.recordCount), sizeof(uint16_t));
    blockFile.read(reinterpret_cast<char*>(&block.precedingRBN), sizeof(uint32_t));
    blockFile.read(reinterpret_cast<char*>(&block.succeedingRBN), sizeof(uint32_t));

    // Read remaining block data
    size_t dataSize = blockSize - 10; // 10 = sizeof(uint16_t) + 2*sizeof(uint32_t)
    block.data.resize(dataSize);
    blockFile.read(block.data.data(), dataSize);

    if (blockFile.gcount() != dataSize) 
    {
        setError("Failed to read block data from file.");
        return block;
    }
    return block;
}

