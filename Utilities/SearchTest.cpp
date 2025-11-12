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
#include "../src/DataManager.h"

const std::string FILE_PATH_IN_RAND = "data/PT2_Randomized.zcb";
const std::string FILE_PATH_SORTED = "data/PT2_Sorted.zcb";

int main()
{
    std::cout << "=== Blocked Sequence Set Test Program ===\n\n";
    
    // Test 1: Read header and index
    std::cout << "--- Test 1: Header and Index Loading ---\n";
    HeaderRecord header;
    HeaderBuffer headerBuffer;

    if(!headerBuffer.readHeader(FILE_PATH_IN_RAND, header))
    {
        std::cerr << "Failed To Read Header From " << FILE_PATH_IN_RAND << std::endl;
        return 1;
    }
    
    std::cout << "Header loaded successfully\n";
    std::cout << "  Block Size: " << header.getBlockSize() << "\n";
    std::cout << "  Block Count: " << header.getBlockCount() << "\n";
    std::cout << "  Record Count: " << header.getRecordCount() << "\n";
    std::cout << "  Stale Flag: " << (header.getStaleFlag() ? "STALE" : "VALID") << "\n\n";

    BlockIndexFile index;

    if(header.getStaleFlag())
    {
        std::cout << "Index is stale - rebuilding...\n";
        if(!(index.createIndexFromBlockedFile(FILE_PATH_IN_RAND, header.getBlockSize(), 
                                        header.getHeaderSize(), header.getSequenceSetListRBN())))
        {
            std::cerr << "Failed To Build Index.\n";
            return 1;
        }
        std::cout << "Index successfully rebuilt.\n\n";
    }
    else
    {
        if(index.read(header.getIndexFileName()))
        {
            std::cout << "Index file exists and was successfully read.\n\n";
        }
        else
        {
            std::cerr << "Failed To Read Existing Index File.\n";
            return 1;
        }
    }

    // Test 2: Search for specific records
    std::cout << "--- Test 2: Record Search ---\n";
    std::vector<uint32_t> zipToFind = {17111, 31836, 8053};

    BlockBuffer blockBuffer;
    if(!blockBuffer.openFile(FILE_PATH_IN_RAND, header.getHeaderSize()))
    {
        std::cerr << "Failed to open block buffer\n";
        return 1;
    }

    for(const auto& zip : zipToFind)
    {
        uint32_t rbn = index.findRBNForKey(zip);
        std::cout << "Searching for ZIP " << zip << " (RBN: " << rbn << ")... ";
        
        ZipCodeRecord tempRecord;
        if(blockBuffer.readRecordAtRBN(rbn, zip, header.getBlockSize(), 
                                      header.getHeaderSize(), tempRecord))
        {
            std::cout << "FOUND\n";
            std::cout << "  " << tempRecord << "\n";
        }
        else
        {
            std::cout << "NOT FOUND\n";
        }
    }
    std::cout << "\n";

    blockBuffer.closeFile();

    // Test 3: Physical dump
    std::cout << "--- Test 3: Physical Order Dump ---\n";
    if(!blockBuffer.openFile(FILE_PATH_IN_RAND, header.getHeaderSize()))
    {
        std::cerr << "Failed to reopen block buffer\n";
        return 1;
    }
    
    blockBuffer.dumpPhysicalOrder(std::cout, header.getSequenceSetListRBN(),
                                  header.getAvailableListRBN(), header.getBlockCount(),
                                  header.getBlockSize(), header.getHeaderSize());
    std::cout << "\n";
    
    // Test 4: Logical dump
    std::cout << "--- Test 4: Logical Order Dump ---\n";
    blockBuffer.dumpLogicalOrder(std::cout, header.getSequenceSetListRBN(),
                                 header.getAvailableListRBN(), header.getBlockSize(),
                                 header.getHeaderSize());
    std::cout << "\n";

    blockBuffer.closeFile();


    std::cout << "\n=== All Tests Completed! ===\n";
    return 0;
}