#include "ZipSearchApp.h"
#include <iostream>

int main(int argc, char* argv[]) 
{
    if (argc < 3) 
    {
        std::cout << "Usage: " << argv[0] << " <datafile.zcd> -Z<zipcode> [-Z<zipcode> ...]\n";
        std::cout << "Example: " << argv[0] << " NotRandomCSV.zcd -Z96737 -Z97134\n";
        return 1;
    }
    
    std::string dataFile = argv[1];
    
    ZipSearchApp app(dataFile);
    
    if (!app.search(argc, argv)) {
        std::cerr << "Search failed.\n";
        return 1;
    }
    
    return 0;
}