#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

#include "../src/BlockBuffer.h"
#include "../src/BlockIndexFile.h"
#include "../src/Block.h"
#include "../src/HeaderBuffer.h"
#include "../src/HeaderRecord.h"
#include "../src/ZipCodeRecord.h"
#include "../src/CSVBuffer.h"
#include "../src/DataManager.h"

const std::string FILE_PATH_IN_RAND = "data/PT2_Randomized.zcb";
const std::string FILE_PATH_SORTED = "data/PT2_Sorted.zcb";

int main()
{
    std::cout << "\n=== Test 1: Blocked File (Randomized Initial Ordered CSV) ===\n";
    DataManager mgr_1;
    size_t records1 = mgr_1.processFromBlockedSequence(FILE_PATH_IN_RAND);
    std::cout << "Processed " << records1 << " records." << std::endl;
    mgr_1.printTable(std::cout);


    std::cout << "\n=== Test 1: Blocked File (Sorted Initial Ordered CSV) ===\n";
    DataManager mgr_2;
    size_t records2 = mgr_2.processFromBlockedSequence(FILE_PATH_SORTED);
    std::cout << "Processed " << records2 << " records." << std::endl;
    mgr_2.printTable(std::cout);

    std::cout << "\n=== Verification Test ===\n";
    bool identical = DataManager::verifyIdenticalResults(FILE_PATH_IN_RAND, FILE_PATH_SORTED, 2, 2);
    std::cout << "[NotRandomCSV.zcb vs Randomized.zcb] Identical? " 
                  << (identical ? "YES" : "NO") << std::endl;

    std::cout << "\n=== All Tests Completed! ===\n";

    return 0;
}