#include "PrimaryKeyIndex.h"
#include "CSVBuffer.h"
#include "ZipCodeRecord.h"
#include "ZipSearchApp.h"
#include "HeaderRecord.h"
#include "HeaderBuffer.h"
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
    if(argc <= 1) return false; 
    std::vector<uint32_t> zips; //zips being searched for
    for (int i = 2; i < argc; ++i) 
    {
        std::string arg = argv[i];// get argument
        if (arg.rfind("-Z", 0) == 0) 
        { //if string argument indicator be found
            uint32_t zip = std::stoi(arg.substr(2)); //get zip code
            zips.push_back(zip); //add zip code to list
        }
    }

    HeaderRecord header;
    HeaderBuffer buffer;

    buffer.readHeader(fileName, header);

    CSVBuffer file;
    if(!file.openLengthIndicatedFile(fileName, header.getHeaderSize())) return false;

    if(header.getHasValidIndexFile() && index.read(header.getIndexFileName()))
    {
        std::cout << "Index file read successfully." << "\n" << std::endl;
        index.read(header.getIndexFileName());
    } 
    else 
    {
        std::cout << "Index file not found or invalid. Creating index from data file: " << fileName << std::endl;
        index.createFromDataFile(file);
        if(index.write("zipcode_data.idx"))
        {
            std::cout << "Index File Written Succesfully." << "\n" << std::endl;

            // Update header flag
            header.setHasValidIndexFile(1);
            header.setIndexFileName("zipcode_data.idx");
            
            // Rewrite just the header section of the file
            std::fstream file(fileName, std::ios::binary | std::ios::in | std::ios::out);
            if (file.is_open()) 
            {
                file.seekp(0); // Go to start
                auto headerData = header.serialize();
                file.write(reinterpret_cast<char*>(headerData.data()), headerData.size());
                file.close();
                std::cout << "Header updated with index file info." << "\n" << std::endl;
            }
        }
        else
        {
            std::cerr << "Failed To Write Index File." << std::endl;
            return false;
        }
       
    }

    for (const auto& zip : zips)
    {
        std::cout << "Searching for zip code: " << zip << std::endl;
        ZipCodeRecord record;
        if (!index.contains(zip)) 
        {
            std::cout << "Zip code " << zip << " not found." << std::endl;
            continue;
        }
        for (const auto& offset : index.find(zip))
        {
            //print the memory offsets associated with the zip code
            std::cout << "Found at memory offset: " << offset << std::endl;
            file.readRecordAtMemoryAddress(offset, record);
            std::cout << "Zip: " << record.getZipCode() 
                << ", Location: " << record.getLocationName()
                << ", State: " << record.getState()
                << ", County: " << record.getCounty()
                << ", Lat: " << record.getLatitude()
                << ", Lon: " << record.getLongitude() << "\n" << std::endl;
            
        }
    }
    return true;
}

