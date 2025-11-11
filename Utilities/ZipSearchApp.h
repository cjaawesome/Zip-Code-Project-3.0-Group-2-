#ifndef ZIP_SEARCH_APP
#define ZIP_SEARCH_APP

#include "PrimaryKeyIndex.h"
#include "BlockIndexFile.h"

#include "CSVBuffer.h"
#include "ZipCodeRecord.h"
#include "BlockBuffer.h"
#include "Block.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>

class ZipSearchApp {
public:
    


    ZipSearchApp();

    ZipSearchApp(const std::string& file);

    void setDataFile(const std::string& file);
    /**
     * @brief parses command line arguments
     * @param argc size of args
     * @param argv argument array
     * @return true if args are successfully parsed
     */
    bool search(int argc, char* argv[]);
    
private:
    std::string fileName;
    PrimaryKeyIndex index;

    /**
     * @brief Parse a CSV string into a ZipCodeRecord
     * @param csv The CSV string (comma-separated fields)
     * @param out The ZipCodeRecord to populate
     * @return true if parsing was successful
     */
    bool parseCsvToRecord(const std::string& csv, ZipCodeRecord& out);

    /**
     * @brief Parse block data into a vector of ZipCodeRecords
     * @param raw The raw block data containing length-prefixed CSV records
     * @param blockRecords Vector to populate with parsed records
     * @return true if parsing was successful
     */
    bool parseBlockData(const std::vector<char>& raw, std::vector<ZipCodeRecord>& blockRecords);
};
#endif