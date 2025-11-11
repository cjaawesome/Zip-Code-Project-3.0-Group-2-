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

    return true;
}
