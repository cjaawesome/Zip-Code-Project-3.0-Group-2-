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

const std::string ADD_ARG = "-A";
const std::string REMOVE_ARG = "-R";
const std::string SEARCH_ARG = "-S";
const std::string ZIP_ARG = "-Z";


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

    // Collect search requested zip codes from command line (-Zxxxxx)
    std::vector<uint32_t> zips;
    argsParser(argc, argv, SEARCH_ARG, zips);

    if (zips.empty()) {
        std::cout << "No zip codes being searched for." << std::endl;
        return false;
    }

    // Read header
    HeaderRecord header;
    HeaderBuffer headerBuffer;
    if (!headerBuffer.readHeader(fileName, header)) {
        std::cerr << "Failed to read header from " << fileName << std::endl;
        return false;
    }
    
    if(!indexHandler(header)){ 
        return false;
    }

    //search for zip codes
    const uint32_t headerSize = header.getHeaderSize();
    const uint32_t blockSize = header.getBlockSize();

    for (const auto& zip : zips) {
        uint32_t rbn = blockIndexFile.findRBNForKey(zip);
        if (rbn == static_cast<uint32_t>(-1)) {
            std::cout << "Zip code " << zip << " not found." << std::endl;
            continue;
        }

        BlockBuffer blockBuffer;
        ZipCodeRecord record;
        if (blockBuffer.openFile(fileName, headerSize) && 
            blockBuffer.readRecordAtRBN(rbn, zip, blockSize, headerSize, record)) {
            std::cout << "Found: " << record.getLocationName() << ", " 
                      << record.getState() << " (" << record.getZipCode() << ")" << std::endl;
        } else {
            std::cout << "Zip code " << zip << " not found in block." << std::endl;
        }
    }

    return true;
}

bool ZipSearchApp::add(int argc, char* argv[]){
    if (argc <= 1) return false;

    // Collect add requested zip codes from command line (-Zxxxxx)
    std::vector<uint32_t> zips;
    argsParser(argc, argv, ADD_ARG, zips);

    if (zips.empty()) {
        std::cout << "No zip codes being added." << std::endl;
        return false;
    }

    // Read header
    HeaderRecord header;
    HeaderBuffer headerBuffer;
    if (!headerBuffer.readHeader(fileName, header)) {
        std::cerr << "Failed to read header from " << fileName << std::endl;
        return false;
    }
    
    if(!indexHandler(header)){ 
        return false;
    }

    //**add the zips to the blocked file */

}


bool ZipSearchApp::remove(int argc, char* argv[]){
    if (argc <= 1) return false;

    // Collect remove requested zip codes from command line (-Zxxxxx)
    std::vector<uint32_t> zips;
    argsParser(argc, argv, REMOVE_ARG, zips);

    if (zips.empty()) {
        std::cout << "No zip codes being removed." << std::endl;
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
    const std::string indexFileName = header.getIndexFileName();
    const uint32_t sequenceSetListRBN = header.getSequenceSetListRBN();
    const bool staleFlag = header.getStaleFlag();
    
    if(!indexHandler(header)){ 
        return false;
    }

    //**removes the zips to the blocked file */

}

bool ZipSearchApp::argsParser(int argc, char* argv[], std::string commandArg, std::vector<uint32_t>& zips){
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind(commandArg, 0) == 0) {
            try {
                i++;
                if (arg.rfind(ZIP_ARG, 0) == 0){
                    uint32_t zip = static_cast<uint32_t>(std::stoul(arg.substr(2)));
                    zips.push_back(zip);
                }
            } catch (...) {
                std::cerr << "Invalid zip flag: " << arg << std::endl;
            }
        }
    }
    return true;
}

bool ZipSearchApp::indexHandler(const HeaderRecord& header){
    const uint32_t blockSize = header.getBlockSize();
    const uint32_t headerSize = header.getHeaderSize();
    const uint32_t blockCount = header.getBlockCount();
    const std::string indexFileName = header.getIndexFileName();
    const uint32_t sequenceSetListRBN = header.getSequenceSetListRBN();
    const bool staleFlag = header.getStaleFlag();
    
    if(staleFlag){
        if(!blockIndexFile.createIndexFromBlockedFile(fileName, blockSize, headerSize, sequenceSetListRBN)){
            std::cerr << "Failed to create index file for " << fileName << std::endl;   
            return false;
        }
        
    }
    else{
        if(!blockIndexFile.read(header.getIndexFileName())){
            std::cerr << "Failed to read index file: " << header.getIndexFileName() << std::endl;
            return false;
        }
    }
    return true;
}