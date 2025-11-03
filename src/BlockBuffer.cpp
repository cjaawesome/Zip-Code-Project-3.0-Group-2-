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

bool BlockBuffer::addRecord(const uint32_t rbn, const uint32_t blockSize, uint32_t& availListRBN, 
                            const ZipCodeRecord& record, const size_t headerSize, uint32_t& blockCount)
{
    ActiveBlock block = loadActiveBlockAtRBN(rbn, blockSize, headerSize); //load block at rbn

    std::vector<ZipCodeRecord> records;
    recordBuffer.unpackBlock(block.data, records); //unpack block data into records
    
    if(block.getTotalSize() + record.getRecordSize() <= blockSize) 
    {
        records.push_back(record); // Add the new record

        std::sort(records.begin(), records.end(), 
            [](const ZipCodeRecord& a, const ZipCodeRecord& b) 
            {
                return a.getZipCode() < b.getZipCode();
            });

        recordBuffer.packBlock(records, block.data, blockSize); // Repack the block data
        block.recordCount = static_cast<uint16_t>(records.size()); // Update record count
        return writeActiveBlockAtRBN(rbn, blockSize, headerSize, block); // Write back to file
    } 
    
    
    if(block.precedingRBN != 0)
    {
        ActiveBlock preceedingBlock = loadActiveBlockAtRBN(block.precedingRBN, blockSize, headerSize);
        if((preceedingBlock.getTotalSize() + records[0].getRecordSize() + 4 <= blockSize) &&
            (block.getTotalSize() + record.getRecordSize() - records[0].getRecordSize() <= blockSize))
        {
            std::vector<ZipCodeRecord> preceedingRecords;
            recordBuffer.unpackBlock(preceedingBlock.data, preceedingRecords);
            ZipCodeRecord temp = records[0];
            preceedingRecords.push_back(temp);
            records.erase(records.begin());
            records.push_back(record);
            std::sort(records.begin(), records.end(), 
                [](const ZipCodeRecord& a, const ZipCodeRecord& b) 
                {
                    return a.getZipCode() < b.getZipCode();
                });
            recordBuffer.packBlock(records, block.data, blockSize);
            recordBuffer.packBlock(preceedingRecords, preceedingBlock.data, blockSize);


            // Original block does not need record change since one was removed and one was added
            preceedingBlock.recordCount = static_cast<uint16_t>(preceedingRecords.size()); // Update record count

            return (writeActiveBlockAtRBN(rbn, blockSize, headerSize, block) && 
            writeActiveBlockAtRBN(block.precedingRBN, blockSize, headerSize, preceedingBlock));
        }
    }

    if(block.succeedingRBN != 0)
    {
        ActiveBlock succeedingBlock = loadActiveBlockAtRBN(block.succeedingRBN, blockSize, headerSize);
        if((succeedingBlock.getTotalSize() + records[records.size() - 1].getRecordSize() + 4 <= blockSize) &&
            (block.getTotalSize() + record.getRecordSize() - records.back().getRecordSize() <= blockSize))
        {
            std::vector<ZipCodeRecord> succeedingRecords;
            recordBuffer.unpackBlock(succeedingBlock.data, succeedingRecords);
            ZipCodeRecord temp = records.back();
            succeedingRecords.insert(succeedingRecords.begin(), temp);
            records.pop_back();
            records.push_back(record);
            std::sort(records.begin(), records.end(), 
                [](const ZipCodeRecord& a, const ZipCodeRecord& b) 
                {
                    return a.getZipCode() < b.getZipCode();
                });
            recordBuffer.packBlock(records, block.data, blockSize);
            recordBuffer.packBlock(succeedingRecords, succeedingBlock.data, blockSize);

            // Original block does not need record change since one was removed and one was added
            succeedingBlock.recordCount = static_cast<uint16_t>(succeedingRecords.size()); // Update record count

            return (writeActiveBlockAtRBN(rbn, blockSize, headerSize, block) && 
            writeActiveBlockAtRBN(block.succeedingRBN, blockSize, headerSize, succeedingBlock));
        }
    }
    
    // Gotta split now no other choice
    uint32_t newRBN = allocateBlock(availListRBN, blockCount, blockSize, headerSize);

    records.push_back(record);
    std::sort(records.begin(), records.end(), 
             [](const ZipCodeRecord& a, const ZipCodeRecord& b) 
             {
                return a.getZipCode() < b.getZipCode();
            });
        
    uint32_t remainder = records.size() / 2; // Truncate for shitty rounding

    std::vector<ZipCodeRecord> splitRecords(records.begin() + remainder, records.end());
    records.erase(records.begin() + remainder, records.end());

    recordBuffer.packBlock(records, block.data, blockSize);

    ActiveBlock splitBlock;
    recordBuffer.packBlock(splitRecords, splitBlock.data, blockSize);

    block.recordCount = static_cast<uint16_t>(records.size());
    splitBlock.recordCount = static_cast<uint16_t>(splitRecords.size());

    splitBlock.precedingRBN = rbn;
    splitBlock.succeedingRBN = block.succeedingRBN;
    block.succeedingRBN = newRBN;

    // Update succeeding block on the split block
    if(splitBlock.succeedingRBN != 0)
    {
        ActiveBlock nextBlock = loadActiveBlockAtRBN(splitBlock.succeedingRBN, blockSize, headerSize);
        nextBlock.precedingRBN = newRBN;
        writeActiveBlockAtRBN(splitBlock.succeedingRBN, blockSize, headerSize, nextBlock);
    }
        
    splitOccurred = true;
    return (writeActiveBlockAtRBN(rbn, blockSize, headerSize, block) && 
            writeActiveBlockAtRBN(newRBN, blockSize, headerSize, splitBlock));
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
    if (!blockFile.is_open()) 
    {
        setError("file not open");
        return block;
    }

    std::streampos offset = headerSize + static_cast<std::streampos>(rbn) * blockSize; //calculate offset of block
    blockFile.seekg(offset); //seek block position

    if (!blockFile.good()) 
    {
        setError("failed to seek RBN number");
        return block;
    }

    // Read the raw block bytes into a temporary buffer
    std::vector<char> raw(blockSize);
    blockFile.read(raw.data(), static_cast<std::streamsize>(blockSize));

    std::streamsize bytesRead = blockFile.gcount();
    if (bytesRead <= 0) 
    {
        setError("Failed to read block from file.");
        return block;
    }

    const size_t metaSize = sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t);
    if (static_cast<size_t>(bytesRead) < metaSize) 
    {
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
    if (static_cast<size_t>(bytesRead) > offsetIdx) 
    {
        block.data.assign(raw.begin() + offsetIdx, raw.begin() + bytesRead);
    } 
    else 
    {
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

uint32_t BlockBuffer::allocateBlock(uint32_t& availListRBN, uint32_t& blockCount, 
                                    const uint32_t blockSize, const size_t headerSize) 
{
    // Try to reuse a freed block first
    if (availListRBN != 0) 
    {
        uint32_t newRBN = availListRBN;
        
        // Pop this block from avail list
        AvailBlock availBlock = loadAvailBlockAtRBN(newRBN, blockSize, headerSize);
        availListRBN = availBlock.succeedingRBN;  // Update head to next available
        
        return newRBN;
    }
    
    // No freed blocks available - allocate new block at end of file
    blockCount++;  // Increment total block count
    return blockCount;  // New block gets this RBN
}

AvailBlock BlockBuffer::loadAvailBlockAtRBN(const uint32_t rbn, const uint32_t blockSize, const size_t headerSize) 
{
    AvailBlock block;
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

    // Read binary metadata
    blockFile.read(reinterpret_cast<char*>(&block.recordCount), sizeof(uint16_t));
    blockFile.read(reinterpret_cast<char*>(&block.succeedingRBN), sizeof(uint32_t));

    // Read/skip padding
    size_t paddingSize = blockSize - sizeof(uint16_t) - sizeof(uint32_t);
    block.padding.resize(paddingSize);
    blockFile.read(block.padding.data(), paddingSize);

    if (!blockFile.good()) 
    {
        setError("Failed to read avail block from file.");
    }
    
    return block;
}