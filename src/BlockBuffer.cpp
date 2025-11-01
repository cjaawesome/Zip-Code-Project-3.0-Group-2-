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

    if(block.getTotalSize() < minBlockSize)
    {
        // Try preceding block first
        if (block.precedingRBN != 0) 
        {
            ActiveBlock precedingBlock = loadActiveBlockAtRBN(block.precedingRBN, blockSize, headerSize);
            std::vector<ZipCodeRecord> precedingRecords;
            recordBuffer.unpackBlock(precedingBlock.data, precedingRecords);
            
            if (tryBorrowFromPreceding(block, precedingBlock, records, precedingRecords, 
                                    blockSize, minBlockSize, headerSize, rbn)) 
            {
                mergeOccurred = true;
                return true;
            }
        }
    
        // Try succeeding block if preceding didn't work
        if (block.succeedingRBN != 0) 
        {
            ActiveBlock succeedingBlock = loadActiveBlockAtRBN(block.succeedingRBN, blockSize, headerSize);
            std::vector<ZipCodeRecord> succeedingRecords;
            recordBuffer.unpackBlock(succeedingBlock.data, succeedingRecords);
            
            if (tryBorrowFromSucceeding(block, succeedingBlock, records, succeedingRecords,
                                        blockSize, minBlockSize, headerSize, rbn)) 
            {
                mergeOccurred = true;
                return true;
            }
        }
    
        // Couldn't borrow from either - write underfull block
        recordBuffer.packBlock(records, block.data, blockSize);
        block.recordCount = static_cast<uint16_t>(records.size());
        return writeActiveBlockAtRBN(rbn, blockSize, headerSize, block);
    }
    else // Block is still acceptable size after removal
    {
        recordBuffer.packBlock(records, block.data, blockSize);
        block.recordCount = static_cast<uint16_t>(records.size());
        return writeActiveBlockAtRBN(rbn, blockSize, headerSize, block);
    }
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
        recordBuffer.packBlock(records, block.data, blockSize); // Repack the block data
        block.recordCount = static_cast<uint16_t>(records.size()); // Update record count
        return writeActiveBlockAtRBN(rbn, blockSize, headerSize, block); // Write back to file
    } 
    // There was not enough room in the given record to add. Now need to try and merge or create new record.
    return false;
}

bool BlockBuffer::getMergeOccurred() const{
    return mergeOccurred;
}

bool BlockBuffer::writeActiveBlockAtRBN(const uint32_t rbn, const uint32_t blockSize, const size_t headerSize, const ActiveBlock& block){

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
    if (!blockFile.is_open()) {
        setError("file not open");
        return block;
    }

    std::streampos offset = headerSize + static_cast<std::streampos>(rbn) * blockSize; //calculate offset of block
    blockFile.seekg(offset); //seek block position

    if (!blockFile.good()) {
        setError("failed to seek RBN number");
        return block;
    }

    // Read the raw block bytes into a temporary buffer
    std::vector<char> raw(blockSize);
    blockFile.read(raw.data(), static_cast<std::streamsize>(blockSize));

    std::streamsize bytesRead = blockFile.gcount();
    if (bytesRead <= 0) {
        setError("Failed to read block from file.");
        return block;
    }

    // The on-disk block layout is expected to begin with metadata fields in this order:
    // [uint16_t recordCount][uint32_t precedingRBN][uint32_t succeedingRBN][payload...]
    const size_t metaSize = sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t);
    if (static_cast<size_t>(bytesRead) < metaSize) {
        setError("Block too small to contain header metadata");
        return block;
    }

    // Copy metadata into the ActiveBlock structure
    size_t offsetIdx = 0;
    memcpy(&block.recordCount, raw.data() + offsetIdx, sizeof(block.recordCount));
    offsetIdx += sizeof(block.recordCount); //reads in block data and adds to offset
    memcpy(&block.precedingRBN, raw.data() + offsetIdx, sizeof(block.precedingRBN));
    offsetIdx += sizeof(block.precedingRBN); //reads in preceding RBN and adds to offset
    memcpy(&block.succeedingRBN, raw.data() + offsetIdx, sizeof(block.succeedingRBN));
    offsetIdx += sizeof(block.succeedingRBN); //reads in succeeding RBN and adds to offset

    // Store the remaining bytes as the payload/data portion of the block
    if (static_cast<size_t>(bytesRead) > offsetIdx) {
        block.data.assign(raw.begin() + offsetIdx, raw.begin() + bytesRead);
    } else {
        block.data.clear();
    }

    return block;
}

bool BlockBuffer::tryBorrowFromPreceding(ActiveBlock& block, ActiveBlock& precedingBlock,
                                        std::vector<ZipCodeRecord>& records,
                                        std::vector<ZipCodeRecord>& precedingRecords,
                                        const uint32_t blockSize, const uint16_t minBlockSize,
                                        const size_t headerSize, const uint32_t rbn)
{
    bool borrowed = false;
    
    for(int i = precedingRecords.size() - 1; i >= 0; --i)
    {
        if((precedingRecords[i].getRecordSize() + block.getTotalSize() + 4 <= blockSize) && 
            (precedingBlock.getTotalSize() - precedingRecords[i].getRecordSize() - 4 >= minBlockSize))
        {
            ZipCodeRecord temp = precedingRecords[i];
            precedingRecords.erase(precedingRecords.begin() + i);
            records.push_back(temp);
            borrowed = true;
        }
        else
        {
            break;
        }
    }
    
    if (!borrowed) return false;
    
    // Sort both vectors
    std::sort(records.begin(), records.end(), 
        [](const ZipCodeRecord& a, const ZipCodeRecord& b) 
        {
            return a.getZipCode() < b.getZipCode();
        });
    std::sort(precedingRecords.begin(), precedingRecords.end(), 
        [](const ZipCodeRecord& a, const ZipCodeRecord& b) 
        {
            return a.getZipCode() < b.getZipCode();
        });
    
    // Pack both blocks
    recordBuffer.packBlock(records, block.data, blockSize);
    recordBuffer.packBlock(precedingRecords, precedingBlock.data, blockSize);
    
    // Update counts
    block.recordCount = static_cast<uint16_t>(records.size());
    precedingBlock.recordCount = static_cast<uint16_t>(precedingRecords.size());
    
    // Write both blocks
    return (writeActiveBlockAtRBN(rbn, blockSize, headerSize, block) && 
            writeActiveBlockAtRBN(block.precedingRBN, blockSize, headerSize, precedingBlock));
}

bool BlockBuffer::tryBorrowFromSucceeding(ActiveBlock& block, ActiveBlock& succeedingBlock,
                                         std::vector<ZipCodeRecord>& records,
                                         std::vector<ZipCodeRecord>& succeedingRecords,
                                         const uint32_t blockSize, const uint16_t minBlockSize,
                                         const size_t headerSize, const uint32_t rbn)
{
    bool borrowed = false;
    
    while(!succeedingRecords.empty())
    {
        if((succeedingRecords[0].getRecordSize() + block.getTotalSize() + 4 <= blockSize) && 
            (succeedingBlock.getTotalSize() - succeedingRecords[0].getRecordSize() - 4 >= minBlockSize))
        {
            ZipCodeRecord temp = succeedingRecords[0];
            succeedingRecords.erase(succeedingRecords.begin());
            records.push_back(temp);
            borrowed = true;
        }
        else
        {
            break;
        }
    }
    
    if (!borrowed) return false;
    
    // Sort both vectors
    std::sort(records.begin(), records.end(), 
        [](const ZipCodeRecord& a, const ZipCodeRecord& b) 
        {
            return a.getZipCode() < b.getZipCode();
        });
    std::sort(succeedingRecords.begin(), succeedingRecords.end(), 
        [](const ZipCodeRecord& a, const ZipCodeRecord& b) 
        {
            return a.getZipCode() < b.getZipCode();
        });
    
    // Pack both blocks
    recordBuffer.packBlock(records, block.data, blockSize);
    recordBuffer.packBlock(succeedingRecords, succeedingBlock.data, blockSize);
    
    // Update counts
    block.recordCount = static_cast<uint16_t>(records.size());
    succeedingBlock.recordCount = static_cast<uint16_t>(succeedingRecords.size());
    
    // Write both blocks
    return (writeActiveBlockAtRBN(rbn, blockSize, headerSize, block) && 
            writeActiveBlockAtRBN(block.succeedingRBN, blockSize, headerSize, succeedingBlock));
}