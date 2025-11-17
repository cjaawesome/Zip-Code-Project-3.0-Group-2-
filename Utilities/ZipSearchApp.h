#ifndef ZIP_SEARCH_APP
#define ZIP_SEARCH_APP

#include "../src/BlockIndexFile.h"

#include "../src/CSVBuffer.h"
#include "../src/ZipCodeRecord.h"
#include "../src/BlockBuffer.h"
#include "../src/Block.h"
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
     * @brief parses command line arguments for search and
     * @param argc size of args
     * @param argv argument array
     * @return true if args are successfully parsed and searched
     */
    bool process(int argc, char* argv[]);

   
    
private:
    std::string fileName;
    BlockIndexFile blockIndexFile;
    bool fileLoaded = false;


    bool indexHandler(const HeaderRecord& header);

     /**
     * @brief searches for a zip code in the blocked file
     * @param zip the zip code to search for
     * @return true if the zip code was found, false otherwise
     */
    bool search(uint32_t zip, uint32_t blockSize, uint32_t headerSize, ZipCodeRecord& outRecord);

    /**
     * @brief adds a zip code to the blocked file
     * @param zip the zip code to add
     * @return true if the zip code was added successfully, false otherwise
     */
    bool add(const ZipCodeRecord zip, HeaderRecord& header);

    /**
     * @brief removes a zip code from the blocked file
     * @param zip the zip code to remove
     * @return true if the zip code was removed successfully, false otherwise
     */
    bool remove(uint32_t zip, HeaderRecord& header);
};
#endif