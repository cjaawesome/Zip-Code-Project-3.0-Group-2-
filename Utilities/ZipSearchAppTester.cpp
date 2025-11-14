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
#include "ZipSearchApp.h"





int main(int argc, char* argv[]){
    ZipSearchApp app("PT2_Randomized.zcb");
    app.process(argc, argv);
}