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
const std::string FILE_ARG = "-F";
const std::string PHYSICAL_DUMP_ARG = "-PD";
const std::string LOGICAL_DUMP_ARG = "-LD";


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
    fileLoaded = true;
    HeaderRecord header;
    HeaderBuffer headerBuffer;
    if (!headerBuffer.readHeader(fileName, header))
    {
        std::cerr << "Failed to read header from " << fileName << std::endl;
    }
    if (!indexHandler(header))
    {
        std::cerr << "Failed to handle index for " << fileName << std::endl;
    }
}

void ZipSearchApp::setDataFile(const std::string& file){
    fileName = file;
    fileLoaded = true;
    HeaderRecord header;
    HeaderBuffer headerBuffer;
    if (!headerBuffer.readHeader(fileName, header))
    {
        std::cerr << "Failed to read header from " << fileName << std::endl;
    }
    if (!indexHandler(header))
    {
        std::cerr << "Failed to handle index for " << fileName << std::endl;
    }
}

bool ZipSearchApp::process(int argc, char* argv[]){

    if (argc <= 1) return false;

    //**parses command line arguments*/
    // File: -F datafile.zcb
    // Search: -S 12345
    // Remove: -R 12345
    // Add: -A 12345 LocationName ST CountyName 40.7128 -74.0060
    // Logical Dump: -LD
    // Physical Dump: -PD
    HeaderRecord header;
    HeaderBuffer headerBuffer;
    uint32_t headerSize;
    uint32_t blockSize;
    uint32_t sequenceSetHead;
    uint32_t availListHead;
    uint32_t blockCount;
    //**if file is loaded, read header and set variables*/
    if(fileLoaded){
        if (!headerBuffer.readHeader(fileName, header)) {
                std::cerr << "Failed to read header from " << fileName << std::endl;
                return false;
            }
            if(!indexHandler(header)){ 
                std::cerr << "Failed to handle index for " << fileName << std::endl;
                return false;
            }
            headerSize = header.getHeaderSize();
            blockSize = header.getBlockSize();
            sequenceSetHead = header.getSequenceSetListRBN();
            availListHead = header.getAvailableListRBN();
            blockCount = header.getBlockCount();
    }

    for (int i = 1; i < argc; ++i) {
        try {
            if(argv[i] == FILE_ARG){
                fileName = argv[++i];

                //read header and set variables
                if (!headerBuffer.readHeader(fileName, header)) {
                    std::cerr << "Failed to read header from " << fileName << std::endl;
                    return false;
                }
                if(!indexHandler(header)){ 
                    std::cerr << "Failed to handle index for " << fileName << std::endl;
                    return false;
                }
                fileLoaded = true;
                headerSize = header.getHeaderSize();
                blockSize = header.getBlockSize();
                sequenceSetHead = header.getSequenceSetListRBN();
                availListHead = header.getAvailableListRBN();
                blockCount = header.getBlockCount();
                std::cout << "Loaded file: " << fileName << std::endl;
            }
            else if(argv[i] == ADD_ARG){
                uint32_t zip = std::stoul(argv[++i]);
                std::string locationName = argv[++i];
                std::string state = argv[++i];
                std::string county = argv[++i];
                double latitude = std::stod(argv[++i]);
                double longitude = std::stod(argv[++i]);
                ZipCodeRecord newRecord(zip, latitude, longitude, locationName, state, county);
                
                //add the new record to the blocked file if possible
                if(!add(newRecord, header)){
                    std::cerr << "Failed to add zip code: " << zip << std::endl;
                    continue;
                }
                std::cout << "Added zip code: " << zip << std::endl;
            }
            else if(argv[i] == SEARCH_ARG){
                uint32_t zip = std::stoul(argv[++i]);
                ZipCodeRecord outRecord;

                //**searches for a zip code in the blocked file */
                if(!search(zip, blockSize, headerSize, outRecord)){
                    std::cout << "Zip code " << zip << " not found in block." << std::endl;
                    continue;
                }
                std::cout << "Found: " << outRecord << std::endl;
            }
            else if(argv[i] == REMOVE_ARG){
                uint32_t zip = std::stoul(argv[++i]);

                //**removes the zips to the blocked file */
                if(!remove(zip, header)){
                    std::cerr << "Failed to remove zip code: " << zip << std::endl;
                    continue;
                }
                std::cout << "Removed zip code: " << zip << std::endl;
            }
            else if(argv[i] == LOGICAL_DUMP_ARG){
                std::string outFile = argv[++i]; //get out file name
                std::ofstream out(outFile, std::ios::out);    
                BlockBuffer blockBuffer;
                if(!blockBuffer.openFile(fileName, headerSize)){
                    std::cerr << "Failed to open block buffer\n";
                    return false;
                }
                blockBuffer.dumpLogicalOrder(out, sequenceSetHead, availListHead, blockSize, headerSize);
                std::cout << "Logical dump written to: " << outFile << std::endl;
            }
            else if(argv[i] == PHYSICAL_DUMP_ARG){
                std::string outFile = argv[++i]; //get out file name
                std::ofstream out(outFile, std::ios::out);    
                BlockBuffer blockBuffer;
                if(!blockBuffer.openFile(fileName, headerSize)){
                    std::cerr << "Failed to open block buffer\n";
                    return false;
                }
                blockBuffer.dumpPhysicalOrder(out, sequenceSetHead, availListHead, blockCount, blockSize, headerSize);
                std::cout << "Physical dump written to: " << outFile << std::endl;
            }
            else {
                std::cerr << "Unknown argument: " << argv[i] << std::endl;
                return false;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error parsing arguments: " << e.what() << std::endl;
            return false;
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