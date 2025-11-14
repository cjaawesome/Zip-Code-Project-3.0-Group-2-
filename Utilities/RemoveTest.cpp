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

const std::string FILE_PATH_IN_RAND = "data/PT2_Randomized.zcb";

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
        std::cout << "Searching for ZIP " << zip << " at RBN: " << rbn << "... " << std::endl;
        
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

    // Test 3: Remove records with detailed diagnostics
    std::cout << "--- Test 3: Remove Records ---\n";
    RecordBuffer recBuf;
    
    for(const auto& zip : zipToFind)
    {
        blockBuffer.resetMerge();
        uint32_t rbn = index.findRBNForKey(zip);
        std::cout << "Trying to remove ZIP " << zip << " at RBN: " << rbn << "... " << std::endl;
        
        // Load and inspect the block BEFORE removal
        ActiveBlock blockBefore = blockBuffer.loadActiveBlockAtRBN(rbn, header.getBlockSize(), header.getHeaderSize());
        std::vector<ZipCodeRecord> recordsBefore;
        recBuf.unpackBlock(blockBefore.data, recordsBefore);
        
        std::cout << "  Block BEFORE removal:\n";
        std::cout << "    Record count: " << recordsBefore.size() << "\n";
        std::cout << "    Block total size: " << blockBefore.getTotalSize() << " / " << header.getBlockSize() << "\n";
        std::cout << "    Utilization: " << (blockBefore.getTotalSize() * 100.0 / header.getBlockSize()) << "%\n";
        
        // Find the record we're about to remove and show its size
        auto it = std::find_if(recordsBefore.begin(), recordsBefore.end(),
                              [zip](const ZipCodeRecord& rec) { return rec.getZipCode() == zip; });
        if(it != recordsBefore.end()) {
            std::cout << "    Record to remove size: " << it->getRecordSize() << " bytes\n";
            std::cout << "    Predicted size after removal: " << (blockBefore.getTotalSize() - it->getRecordSize() - 4) << " bytes\n";
            std::cout << "    Min block size threshold: " << header.getMinBlockSize() << " bytes\n";
        }
        
        uint32_t availListRBN = header.getAvailableListRBN();
        if(blockBuffer.removeRecordAtRBN(rbn, header.getMinBlockSize(), availListRBN,
                                            zip, header.getBlockSize(), header.getHeaderSize()))
        {
            std::cout << "SUCCESS" << std::endl;
            if(blockBuffer.getMergeOccurred()) 
            {
                std::cout << "  TRUE MERGE occurred (block deleted)\n";
                
                // Check adjacent block sizes
                if(blockBefore.precedingRBN != 0) {
                    ActiveBlock prec = blockBuffer.loadActiveBlockAtRBN(blockBefore.precedingRBN, header.getBlockSize(), header.getHeaderSize());
                    std::cout << "    Preceding block (RBN " << blockBefore.precedingRBN << ") size was: " << prec.getTotalSize() << "\n";
                }
                if(blockBefore.succeedingRBN != 0) {
                    ActiveBlock succ = blockBuffer.loadActiveBlockAtRBN(blockBefore.succeedingRBN, header.getBlockSize(), header.getHeaderSize());
                    std::cout << "    Succeeding block (RBN " << blockBefore.succeedingRBN << ") size was: " << succ.getTotalSize() << "\n";
                }
            }
        }
        else
        {
             std::cout << "FAILED" << std::endl;
        }
        
        if(availListRBN != header.getAvailableListRBN()) 
        {
            std::cout << "  Avail list updated: " << header.getAvailableListRBN() << " -> " << availListRBN << "\n";
            header.setAvailableListRBN(availListRBN);
        }
        std::cout << "\n";
    }

    // Test 4: Verify removal
    std::cout << "--- Test 4: Verify Records Were Removed ---\n";
    for(const auto& zip : zipToFind)
    {
        uint32_t rbn = index.findRBNForKey(zip);
        std::cout << "Searching for ZIP " << zip << " at RBN: " << rbn << "... " << std::endl;
        
        ZipCodeRecord tempRecord;
        if(blockBuffer.readRecordAtRBN(rbn, zip, header.getBlockSize(), 
                                      header.getHeaderSize(), tempRecord))
        {
            std::cout << "FOUND (ERROR!)\n";
            std::cout << "  " << tempRecord << "\n";
        }
        else
        {
            std::cout << "NOT FOUND (correct)\n";
        }
    }

    blockBuffer.closeFile();
    std::cout << "\n=== Tests Completed Successfully! ===\n";
    return 0;
}