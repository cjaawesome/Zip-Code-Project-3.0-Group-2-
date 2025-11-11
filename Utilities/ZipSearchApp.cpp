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
    HeaderBuffer headerBuf;
    if (!headerBuf.readHeader(fileName, header)) {
        std::cerr << "Failed to read header from " << fileName << std::endl;
        return false;
    }

    const uint32_t blockSize = header.getBlockSize();
    const uint32_t headerSize = header.getHeaderSize();

    // Use BlockIndexFile to store simple index entries {highestKey, RBN}
    std::string simpleIndexFile = fileName + ".sidx";
    BlockIndexFile bix;

    if (!bix.read(simpleIndexFile)) {
        // Build from data file by scanning records and tracking highest key per RBN
        CSVBuffer dataBuf;
        if(!dataBuf.openLengthIndicatedFile(fileName, headerSize)){
            std::cerr << "Failed to open data file for indexing." << std::endl;
            return false;
        }

        std::map<uint32_t, uint32_t> highestPerRbn; // rbn -> highest key

        ZipCodeRecord rec;
        size_t dataOffset = dataBuf.getMemoryOffset();
        while (dataBuf.getNextLengthIndicatedRecord(rec)) {
            uint32_t zip = rec.getZipCode();
            if (dataOffset < headerSize) dataOffset = headerSize;
            uint32_t rbn = static_cast<uint32_t>((dataOffset - headerSize) / blockSize);
            auto it = highestPerRbn.find(rbn);
            if (it == highestPerRbn.end() || zip > it->second) highestPerRbn[rbn] = zip;
            dataOffset = dataBuf.getMemoryOffset();
        }
        // Add entries to BlockIndexFile. addIndexEntry will keep ordering.
        for (const auto &kv : highestPerRbn) {
            IndexEntry ie;
            ie.key = kv.second; // highest zip
            ie.recordRBN = kv.first; // rbn
            bix.addIndexEntry(ie);
        }
        if (!bix.write(simpleIndexFile)) {
            std::cerr << "Failed to write simple index file: " << simpleIndexFile << std::endl;
        } else {
            std::cout << "Built simple primary key index (" << highestPerRbn.size() << " entries)" << std::endl;
        }
    }

    // Open block file reader
    BlockBuffer blockBuf;
    if(!blockBuf.openFile(fileName, headerSize)){
        std::cerr << "Failed to open block file for reading: " << fileName << std::endl;
        return false;
    }

    // For each searched zip, find block via simple index then scan that block
    for (uint32_t zip : zips) {
        std::cout << "Searching for zip code: " << zip << std::endl;

        // Ask BlockIndexFile for the RBN
        uint32_t rbn = bix.findRBNForKey(zip);
        if (rbn == static_cast<uint32_t>(-1)) {
            std::cout << "Zip code " << zip << " not found in index." << std::endl;
            continue;
        }
        ActiveBlock ab = blockBuf.loadActiveBlockAtRBN(rbn, blockSize, headerSize);

        // parse ab.data into a vector of ZipCodeRecords
        std::vector<ZipCodeRecord> blockRecords;
        if (!parseBlockData(ab.data, blockRecords)) {
            std::cerr << "Failed to parse block data for RBN " << rbn << std::endl;
            continue;
        }

        // search vector for exact zip match
        bool found = false;
        for (const auto &r : blockRecords) {
            if (r.getZipCode() == zip) {
                found = true;
                std::cout << "Found: ZIP=" << r.getZipCode()
                    << ", Location=" << r.getLocationName()
                    << ", State=" << r.getState()
                    << ", County=" << r.getCounty()
                    << ", Lat=" << r.getLatitude()
                    << ", Lon=" << r.getLongitude() << std::endl;
                break;
            }
        }
        if (!found) std::cout << "Zip code " << zip << " not found in block " << rbn << std::endl;
    }

    return true;
}

bool ZipSearchApp::parseCsvToRecord(const std::string& csv, ZipCodeRecord& out)
{
    std::vector<std::string> fields;
    size_t start = 0;
    while (start <= csv.size()) {
        size_t pos = csv.find(',', start);
        if (pos == std::string::npos) pos = csv.size();
        fields.push_back(csv.substr(start, pos - start));
        start = pos + 1;
        if (pos == csv.size()) break;
    }
    if (fields.size() != 6) return false;
    try {
        uint32_t zip = static_cast<uint32_t>(std::stoul(fields[0]));
        double lat = std::stod(fields[4]);
        double lon = std::stod(fields[5]);
        out = ZipCodeRecord(static_cast<int>(zip), lat, lon, fields[1], fields[2], fields[3]);
        return true;
    } catch (...) {
        return false;
    }
}

bool ZipSearchApp::parseBlockData(const std::vector<char>& raw, std::vector<ZipCodeRecord>& blockRecords)
{
    blockRecords.clear();
    size_t pos = 0;
    while (pos + 4 <= raw.size()) {
        uint32_t recLen = 0;
        memcpy(&recLen, raw.data() + pos, sizeof(recLen));
        pos += 4;
        if (recLen == 0 || pos + recLen > raw.size()) break;
        std::string csv(recLen, '\0');
        memcpy(&csv[0], raw.data() + pos, recLen);
        pos += recLen;
        ZipCodeRecord r;
        if (parseCsvToRecord(csv, r)) {
            blockRecords.push_back(r);
        }
    }
    return true;
}

