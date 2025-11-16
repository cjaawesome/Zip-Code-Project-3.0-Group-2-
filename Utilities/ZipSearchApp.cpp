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


// uint32_t zipCode; // 5-digit zip code
// std::string locationName; // Town name
// std::string county; // County name
// char state[3]; // Two-character state code + null terminator
// double latitude; // Latitude coordinate
// double longitude; // Longitude coordinate  

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
    std::vector<ZipCodeRecord> addZips;
    std::vector<uint32_t> removeZips;

    //**parses command line arguments*/
    // Search: -S 12345
    // Remove: -R 12345
    // Add: -A 12345 LocationName ST CountyName 40.7128 -74.0060
    
    for (int i = 0; i < argc; ++i) {
        try {
            if(argv[i] == ADD_ARG){
                uint32_t zip = std::stoul(argv[++i]);
                std::string locationName = argv[++i];
                std::string state = argv[++i];
                std::string county = argv[++i];
                double latitude = std::stod(argv[++i]);
                double longitude = std::stod(argv[++i]);
                addZips.emplace_back(zip, latitude, longitude, locationName, state, county);
            }
            else if(argv[i] == SEARCH_ARG){
                uint32_t zip = std::stoul(argv[++i]);
                searchZips.push_back(zip);
            }
            else if(argv[i] == REMOVE_ARG){
                uint32_t zip = std::stoul(argv[++i]);
                removeZips.push_back(zip);
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error parsing arguments: " << e.what() << std::endl;
            return false;
        }
    }

    //** read header */
    HeaderRecord header;
    HeaderBuffer headerBuffer;
    if (!headerBuffer.readHeader(fileName, header)) {
        std::cerr << "Failed to read header from " << fileName << std::endl;
        return false;
    }
    
    //** create index file if stale, otherwise read it */
    if(!indexHandler(header)){ 
        return false;
    }

    //search for zip codes
    const uint32_t headerSize = header.getHeaderSize();
    const uint32_t blockSize = header.getBlockSize();

    //**searches for a zip code in the blocked file */
    if(searchZips.empty()){
        std::cout << "No zip codes provided for search." << std::endl;
    }
    else{
        for(auto& zip : searchZips){
            ZipCodeRecord outRecord;
            if(!search(zip, blockSize, headerSize, outRecord)){
                std::cout << "Zip code " << zip << " not found in block." << std::endl;
                continue;
            }
            std::cout << "Found: " << outRecord.getLocationName() << ", " 
                      << outRecord.getState() << " (" << outRecord.getZipCode() << ")" << std::endl;
        }
    }

    //**add the zips to the blocked file */
    if(addZips.empty()){
        std::cout << "No zip codes provided for addition." << std::endl;
    }
    else{
        for(auto& zip : addZips){
            if(!add(zip, header)){
                std::cerr << "Failed to add zip code: " << zip << std::endl;
                continue;
            }
            std::cout << "Added zip code: " << zip << std::endl;
        }
    }

    //**removes the zips to the blocked file */
    if(removeZips.empty()){
        std::cout << "No zip codes provided for removal." << std::endl;
    }
    else{
        for(auto& zip : removeZips){
            if(!remove(zip, header)){
                std::cerr << "Failed to remove zip code: " << zip << std::endl;
                continue;
            }
            std::cout << "Removed zip code: " << zip << std::endl;
        }
    }


    return true;

}


bool ZipSearchApp::search(uint32_t zip, uint32_t blockSize, uint32_t headerSize, ZipCodeRecord& outRecord){
    //**searches for a zip code in the blocked file */
    uint32_t rbn = blockIndexFile.findRBNForKey(zip);
    if (rbn == static_cast<uint32_t>(-1)) {
        std::cout << "Zip code " << zip << " not found." << std::endl;
        return false;
    }

    //**searches for a zip code in the blocked file */
    BlockBuffer blockBuffer;
    ZipCodeRecord record;
    if (blockBuffer.openFile(fileName, headerSize) && 
        blockBuffer.readRecordAtRBN(rbn, zip, blockSize, headerSize, record)) {
        outRecord = record;
    } else {
        return false;
    }
    return true;
}

bool ZipSearchApp::add(const ZipCodeRecord zip, HeaderRecord& header){
    BlockBuffer blockBuffer;
    if(!blockBuffer.openFile(fileName, header.getHeaderSize()))
    {
        std::cerr << "Failed to open block buffer\n";
        return 1;
    }
    uint32_t blockCount = header.getBlockCount();
    uint32_t availListRBN = header.getAvailableListRBN();

    blockBuffer.resetSplit();
    uint32_t rbn = blockIndexFile.findRBNForKey(zip.getZipCode());
    
    if(!blockBuffer.addRecord(rbn, header.getBlockSize(), availListRBN, zip, 
                            header.getHeaderSize(), blockCount))
    {
        return false;
    }
    return true;
}



bool ZipSearchApp::remove(uint32_t zip, HeaderRecord& header){
    
    //**checks if the zip code exists in the blocked file */
    ZipCodeRecord record;
    uint32_t blockSize = header.getBlockSize();
    uint32_t headerSize = header.getHeaderSize();
    if (!search(zip, blockSize, headerSize, record)) {
        return false;
    }

    //**removes the zips to the blocked file */

    BlockBuffer blockBuffer;
    RecordBuffer recordBuffer;
    blockBuffer.openFile(fileName, headerSize);


    blockBuffer.resetMerge();
    uint32_t rbn = blockIndexFile.findRBNForKey(zip);
        
    // Load and inspect the block BEFORE removal
    ActiveBlock blockBefore = blockBuffer.loadActiveBlockAtRBN(rbn, header.getBlockSize(), header.getHeaderSize());
    std::vector<ZipCodeRecord> recordsBefore;
    recordBuffer.unpackBlock(blockBefore.data, recordsBefore);
        
    // Find the record we're about to remove and show its size
    auto it = std::find_if(recordsBefore.begin(), recordsBefore.end(),
                          [zip](const ZipCodeRecord& rec) { return rec.getZipCode() == zip; });
    
        
    uint32_t availListRBN = header.getAvailableListRBN();
    if(blockBuffer.removeRecordAtRBN(rbn, header.getMinBlockSize(), availListRBN,
                                        zip, header.getBlockSize(), header.getHeaderSize()))
    {
        if(blockBuffer.getMergeOccurred()) 
        {
            
            // Check adjacent block sizes
            if(blockBefore.precedingRBN != 0) {
                ActiveBlock prec = blockBuffer.loadActiveBlockAtRBN(blockBefore.precedingRBN, header.getBlockSize(), header.getHeaderSize());
            }
            if(blockBefore.succeedingRBN != 0) {
                ActiveBlock succ = blockBuffer.loadActiveBlockAtRBN(blockBefore.succeedingRBN, header.getBlockSize(), header.getHeaderSize());
            }
        }
      }
    else
    {
         return false;
    }
        
    if(availListRBN != header.getAvailableListRBN()) 
    {
        header.setAvailableListRBN(availListRBN);
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