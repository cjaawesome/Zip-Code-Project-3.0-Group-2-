#ifndef ZIP_SEARCH_APP
#define ZIP_SEARCH_APP

#include "../src/PrimaryKeyIndex.h"
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
     * @brief parses command line arguments
     * @param argc size of args
     * @param argv argument array
     * @return true if args are successfully parsed
     */
    bool search(int argc, char* argv[]);
    
private:
    std::string fileName;
    PrimaryKeyIndex index;

};
#endif