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

bool ZipSearchApp::process(int argc, char* argv[]){

    if (argc <= 1) return false;

    std::vector<uint32_t> searchZips;
    std::vector<uint32_t> addZips;
    std::vector<uint32_t> removeZips;

    bool commandFound = false;
    std::string currentCommand = "";
    for (int i = 0; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind(ADD_ARG, 0) == 0) {
            commandFound = true;
            currentCommand = ADD_ARG;
        }
        else if (arg.rfind(REMOVE_ARG, 0) == 0) {
            commandFound = true;
            currentCommand = REMOVE_ARG;
        }
        else if (arg.rfind(SEARCH_ARG, 0) == 0) {
            commandFound = true;
            currentCommand = SEARCH_ARG;
        }
        else if (commandFound) {
            try {
                if (arg.rfind(ZIP_ARG, 0) == 0){
                    uint32_t zip = static_cast<uint32_t>(std::stoul(arg.substr(2)));
                    if(currentCommand == ADD_ARG)
                        addZips.push_back(zip);
                    else if(currentCommand == REMOVE_ARG)
                        removeZips.push_back(zip);
                    else if(currentCommand == SEARCH_ARG)
                        searchZips.push_back(zip);
                }
            } catch (...) {
                std::cerr << "Invalid zip flag: " << arg << std::endl;
            }
            commandFound = false; // Reset for next command
        }
        else {
            std::cerr << "Unknown argument: " << arg << std::endl;
        }
    }

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

    if(searchZips.empty()){
        std::cout << "No zip codes provided for search." << std::endl;
    }
    else{
        for(auto& zip : searchZips){
            ZipCodeRecord outRecord;
            if(!search(zip, blockSize, headerSize, outRecord)){
                std::cerr << "Failed to search for zip code: " << zip << std::endl;
                continue;
            }
            std::cout << "Found: " << outRecord.getLocationName() << ", " 
                      << outRecord.getState() << " (" << outRecord.getZipCode() << ")" << std::endl;
        }
    }

    if(addZips.empty()){
        std::cout << "No zip codes provided for addition." << std::endl;
    }
    else{
        for(auto& zip : addZips){
            if(!add(zip)){
                std::cerr << "Failed to add zip code: " << zip << std::endl;
                continue;
            }
            std::cout << "Added zip code: " << zip << std::endl;
        }
    }

    if(removeZips.empty()){
        std::cout << "No zip codes provided for removal." << std::endl;
    }
    else{
        for(auto& zip : removeZips){
            if(!remove(zip)){
                std::cerr << "Failed to remove zip code: " << zip << std::endl;
                continue;
            }
            std::cout << "Removed zip code: " << zip << std::endl;
        }
    }


    return true;

}


bool ZipSearchApp::search(uint32_t zip, uint32_t blockSize, uint32_t headerSize, ZipCodeRecord& outRecord){
    uint32_t rbn = blockIndexFile.findRBNForKey(zip);
    if (rbn == static_cast<uint32_t>(-1)) {
        std::cout << "Zip code " << zip << " not found." << std::endl;
        return false;
    }

    BlockBuffer blockBuffer;
    ZipCodeRecord record;
    if (blockBuffer.openFile(fileName, headerSize) && 
        blockBuffer.readRecordAtRBN(rbn, zip, blockSize, headerSize, record)) {
        outRecord = record;
    } else {
        std::cout << "Zip code " << zip << " not found in block." << std::endl;
        return false;
    }
    return true;
}

bool ZipSearchApp::add(uint32_t zip){
    
    
    //**adds the zips to the blocked file */

    return true;
}



bool ZipSearchApp::remove(uint32_t zip){
    
    //**removes the zips to the blocked file */
    
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