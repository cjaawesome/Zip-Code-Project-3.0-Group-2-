#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

#include "../src/BlockBuffer.h"
#include "../src/BlockIndexFile.h"
#include "../src/Block.h"
#include "../src/HeaderBuffer.h"
#include "../src/HeaderRecord.h"
#include "../src/ZipCodeRecord.h"
#include "../src/CSVBuffer.h"

const std::string FILE_PATH_IN = "PT2_Randomized.csv";
const std::string FILE_PATH_OUT = "PT2_Randomized.zcd";
const std::string& FILE_PATH_INDEX = "PT2_Randomized.idx";

//from ZCDUtility.cpp
bool convertCSVToBlockedSequenceSet(const std::string& csvFile, const std::string& zcbFile, const std::string& indexFile, 
                                    uint32_t blockSize = 1024, uint16_t minBlockSize = 256)
{
    CSVBuffer csvBuffer;
    if(!csvBuffer.openFile(csvFile))
    {
        std::cerr << "Failed to open CSV file." << std::endl;
        return false;
    }

    std::vector<ZipCodeRecord> allRecords;
    ZipCodeRecord record;
    while(csvBuffer.getNextRecord(record))
    {
        allRecords.push_back(record);
    }

    std::cout << "Read " << allRecords.size() << " records." << std::endl;

    std::sort(allRecords.begin(), allRecords.end(),
                [](const ZipCodeRecord& a, const ZipCodeRecord& b)
            {
                return a.getZipCode() < b.getZipCode();
            });

    std:: cout << "Sorted records by ZipCode." << std::endl;

    HeaderRecord header;

    header.setFileStructureType("ZIPC");
    header.setVersion(2);
    header.setHeaderSize(0); // Set In Serialization Process
    header.setSizeFormatType(0);
    header.setBlockSize(blockSize);
    header.setMinBlockSize(minBlockSize);
    header.setIndexFileName("zipcode_data.idx"); // Placeholder
    header.setIndexFileSchemaInfo("Primary Key: Zipcode"); // Placeholder
    header.setRecordCount(allRecords.size()); // This will need to be tracked and updated after if sorting changes
    header.setBlockCount(0); // Update After Conversion
    
    std::vector<FieldDef> fields;
    fields.push_back({"zipcode", 1});
    fields.push_back({"location", 3});
    fields.push_back({"state", 4});
    fields.push_back({"county", 3});
    fields.push_back({"latitude", 2});
    fields.push_back({"longitude", 2});
    
    header.setFields(fields);
    header.setFieldCount(csvBuffer.EXPECTED_FIELD_COUNT);
    header.setPrimaryKeyField(0);
    header.setAvailableListRBN(0); 
    header.setSequenceSetListRBN(1); 
    header.setStaleFlag(0);

    std::ofstream out(zcbFile, std::ios::binary);
    if (!out.is_open()) 
    {
        std::cerr << "Error: Cannot create output file: " << zcbFile << std::endl;
        return false;
    }
    
    auto headerData = header.serialize();
    header.setHeaderSize(headerData.size());
    
    out.write(reinterpret_cast<char*>(headerData.data()), headerData.size());
    
    size_t blockCountOffset = 4 + 2 + 4 + 1 + 4 + 2 + 2 + 
                         header.getIndexFileName().length() + 
                         2 + header.getIndexFileSchemaInfo().length() + 
                         sizeof(uint32_t);
    
    std::cout << "Converting " << csvFile << " to " << zcbFile << "..." << std::endl;

    RecordBuffer recordBuffer;
    BlockBuffer blockBuffer;
    BlockIndexFile blockIndexFile;

    if(!blockBuffer.openFile(zcbFile, header.getHeaderSize()))
    {
        std::cerr << "Failed to open block buffer." << std::endl;    
    }

    uint32_t currentRBN = 1;
    uint32_t blockCount = 0;
    std::vector<ZipCodeRecord> currentBlockRecords;
    size_t currentSize = 10;  // metadata
    

    for(const auto& rec : allRecords)
    {
        // Check if adding this record would overflow
        if (currentSize + rec.getRecordSize() + 4 > blockSize)
        {
            // Write current block
            ActiveBlock block;
            IndexEntry entry;
            block.precedingRBN = (currentRBN == 1) ? 0 : currentRBN - 1;
            block.succeedingRBN = currentRBN + 1;  // Temp - will fix last block later
            block.recordCount = static_cast<uint16_t>(currentBlockRecords.size());
            
            recordBuffer.packBlock(currentBlockRecords, block.data, blockSize);
            blockBuffer.writeActiveBlockAtRBN(currentRBN, blockSize, header.getHeaderSize(), block);

            entry.key = currentBlockRecords.front().getZipCode();
            entry.recordRBN = currentRBN;
            entry.previousRBN = block.precedingRBN;
            entry.nextRBN = block.succeedingRBN;
            
            blockIndexFile.addIndexEntry(entry);

            ++blockCount;
            ++currentRBN;
            currentBlockRecords.clear();
            currentSize = 10;  // Reset to metadata size
        }

        //write index file
        blockIndexFile.write(indexFile);

        // Add record to current block
        currentBlockRecords.push_back(rec);
        currentSize += rec.getRecordSize() + 4;
    }

    // Write final block
    if (!currentBlockRecords.empty())
    {
        ActiveBlock block;
        block.precedingRBN = (currentRBN == 1) ? 0 : currentRBN - 1;
        block.succeedingRBN = 0;  // Last block
        block.recordCount = static_cast<uint16_t>(currentBlockRecords.size());
        
        recordBuffer.packBlock(currentBlockRecords, block.data, blockSize);
        blockBuffer.writeActiveBlockAtRBN(currentRBN, blockSize, header.getHeaderSize(), block);
        
        ++blockCount;
    }

    out.seekp(blockCountOffset);
    out.write(reinterpret_cast<char*>(&blockCount), sizeof(uint32_t));

    // Once index are setup creating the index and setting the stale flag will go here.

    csvBuffer.closeFile();
    blockBuffer.closeFile();

    return true;
}

int main()
{
    convertCSVToBlockedSequenceSet(FILE_PATH_IN, FILE_PATH_OUT, FILE_PATH_INDEX);
    return 0;
}
