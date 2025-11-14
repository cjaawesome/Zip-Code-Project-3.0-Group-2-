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
#include "../src/RecordBuffer.h"

const std::string FILE_PATH = "data/PT2_Randomized.zcb";

int main()
{
    std::cout << "=== Add Records Test Program ===\n\n";
    
    // Load header and index
    std::cout << "--- Loading Header and Index ---\n";
    HeaderRecord header;
    HeaderBuffer headerBuffer;

    if(!headerBuffer.readHeader(FILE_PATH, header))
    {
        std::cerr << "Failed to read header\n";
        return 1;
    }
    
    std::cout << "Header loaded successfully\n";
    std::cout << "  Block Count: " << header.getBlockCount() << "\n\n";

    BlockIndexFile index;
    if(!index.read(header.getIndexFileName()))
    {
        std::cerr << "Failed to read index\n";
        return 1;
    }

    BlockBuffer blockBuffer;
    if(!blockBuffer.openFile(FILE_PATH, header.getHeaderSize()))
    {
        std::cerr << "Failed to open block buffer\n";
        return 1;
    }

    // Test: Add 3 new records
    std::cout << "--- Adding New Records ---\n";
    std::vector<ZipCodeRecord> recordsToAdd = 
    {
        ZipCodeRecord(50000, 45.0, -93.0, "Test City 1", "MN", "Test County 1"),
        ZipCodeRecord(30000, 40.0, -74.0, "Test City 2", "NY", "Test County 2"),
        ZipCodeRecord(70000, 34.0, -118.0, "Test City 3", "CA", "Test County 3")
    };
    
    uint32_t blockCount = header.getBlockCount();
    uint32_t availListRBN = header.getAvailableListRBN();
    
    for(const auto& rec : recordsToAdd)
    {
        blockBuffer.resetSplit();
        uint32_t rbn = index.findRBNForKey(rec.getZipCode());
        std::cout << "Adding ZIP " << rec.getZipCode() << " to RBN " << rbn << "... ";
        
        if(blockBuffer.addRecord(rbn, header.getBlockSize(), availListRBN, rec, 
                                header.getHeaderSize(), blockCount))
        {
            std::cout << "SUCCESS\n";
            if(blockBuffer.getSplitOccurred()) 
            {
                std::cout << "  Block split occurred (new block count: " << blockCount << ")\n";
            }
        }
        else
        {
            std::cout << "FAILED\n";
        }
    }
    std::cout << "\n";

    // Test: Verify additions
    std::cout << "--- Verifying Added Records ---\n";
    for(const auto& rec : recordsToAdd)
    {
        uint32_t rbn = index.findRBNForKey(rec.getZipCode());
        std::cout << "Searching for ZIP " << rec.getZipCode() << " at RBN " << rbn << "... ";
        
        ZipCodeRecord foundRecord;
        if(blockBuffer.readRecordAtRBN(rbn, rec.getZipCode(), header.getBlockSize(), 
                                      header.getHeaderSize(), foundRecord))
        {
            std::cout << "FOUND\n";
            std::cout << "  " << foundRecord << "\n";
        }
        else
        {
            std::cout << "NOT FOUND\n";
        }
    }

    blockBuffer.closeFile();
    std::cout << "\n=== Test Completed! ===\n";
    return 0;
}