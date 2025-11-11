#include "../src/PrimaryKeyIndex.h"
#include "../src/CSVBuffer.h"
#include "../src/ZipCodeRecord.h"
#include "../src/HeaderRecord.h"
#include "../src/HeaderBuffer.h"
#include "ZipSearchApp.h"
#include <iostream>
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
        if(!blockBuffer.openFile(fileName, headerSize)){
            std::cerr << "Failed to open block buffer." << std::endl;    
        }
        for(size_t i = 1; i < blockCount; ++i){
            for(const auto& zip : zips){
                ZipCodeRecord record;
                if(blockBuffer.readRecordAtRBN(i, zip, blockSize, headerSize, record)){
                    /*
                    Need to figure out how to get highest zip code from each block

                    build a block index for block that contains the zip being searched for if not in the index file
                    */
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
