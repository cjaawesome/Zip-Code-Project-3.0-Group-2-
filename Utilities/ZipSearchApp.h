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
     * @brief parses command line arguments for search
     * @param argc size of args
     * @param argv argument array
     * @return true if args are successfully parsed and searched
     */
    bool search(int argc, char* argv[]);

    /**
     * @brief parses command line arguments for add
     * @param argc size of args
     * @param argv argument array
     * @return true if args are successfully parsed and added
     */
    bool add(int argc, char* argv[]);

    /**
     * @brief parses command line arguments for remove
     * @param argc size of args
     * @param argv argument array
     * @return true if args are successfully parsed and removed
     */
    bool remove(int argc, char* argv[]);
    
private:
    std::string fileName;
    BlockIndexFile blockIndexFile;

    bool argsParser(int argc, char* argv[], std::string commandArg, std::vector<uint32_t>& zips);

    bool indexHandler(const HeaderRecord& header);
};
#endif