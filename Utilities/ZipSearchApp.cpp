#include "../src/PrimaryKeyIndex.h"
#include "../src/CSVBuffer.h"
#include "../src/ZipCodeRecord.h"
#include "../src/HeaderRecord.h"
#include "../src/HeaderBuffer.h"
#include "ZipSearchApp.h"
#include <iostream>
#include <sstream>
#include <map>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>

ZipSearchApp::ZipSearchApp(){

}

ZipSearchApp::ZipSearchApp(const std::string& file){
    fileName = file;
}

void ZipSearchApp::setDataFile(const std::string& file){
    fileName = file;
}

bool ZipSearchApp::search(int argc, char* argv[])
{
    if (argc <= 1) return false;

    // Collect requested zip codes from command line (-Zxxxxx)
    std::vector<uint32_t> zips;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("-Z", 0) == 0) {
            try {
                uint32_t zip = static_cast<uint32_t>(std::stoul(arg.substr(2)));
                zips.push_back(zip);
            } catch (...) {
                std::cerr << "Invalid zip flag: " << arg << std::endl;
            }
        }
    }

    if (zips.empty()) {
        std::cerr << "No zip codes supplied (use -Zxxxxx flags)." << std::endl;
        return false;
    }

    // Read header
    HeaderRecord header;
    HeaderBuffer headerBuffer;
    if (!headerBuffer.readHeader(fileName, header)) {
        std::cerr << "Failed to read header from " << fileName << std::endl;
        return false;
    }
    
    const uint32_t blockSize = header.getBlockSize();
    const uint32_t headerSize = header.getHeaderSize();
    const uint32_t blockCount = header.getBlockCount();
    BlockIndexFile blockIndexFile;

    if(!header.getStaleFlag()){
        BlockBuffer blockBuffer;
        //open block file
        if(!blockBuffer.openFile(fileName, headerSize)){
            std::cerr << "Failed to open block buffer." << std::endl;   
            return false;
        }

        ZipCodeRecord record;
        IndexEntry entry;
        ActiveBlock block;
        RecordBuffer recordBuffer;
        std::vector<ZipCodeRecord> records;
        
        //Logic for writing index file for un indexed block file
        for(size_t i = 1; i < blockCount; ++i){
            block = blockBuffer.loadActiveBlockAtRBN(i, blockSize, headerSize);
            
            for(const auto& zip : zips){
                //if current zip "zip" is found in current block "i" and current zip is not in index file
                if(blockBuffer.readRecordAtRBN(i, zip, blockSize, headerSize, record) && (blockIndexFile.findRBNForKey(zip) != -1)){
                    //get largest zip code in block and set it to key
                    recordBuffer.unpackBlock(block.data, records); 
                    entry.key = records.back().getZipCode();  
                    //set recordRBN to current block "i"
                    entry.recordRBN = i;
                    blockIndexFile.addIndexEntry(entry);
                    //clears vector
                    records.clear();
                }
            }
        }
    }
    else{
        if(!blockIndexFile.read(header.getIndexFileName())){
            std::cerr << "Failed to read index file: " << header.getIndexFileName() << std::endl;
            return false;
        }
    }

    /*
    search index file for zip codes
    */

    /*
    pack up new index file if needed.
    */

    return true;
}
