// ZCDUtility.cpp
#include "../src/HeaderRecord.h"
#include "../src/HeaderBuffer.h"
#include "../src/CSVBuffer.h"
#include "../src/ZipCodeRecord.h"
#include "../src/PrimaryKeyIndex.h"
#include "../src/BlockBuffer.h"
#include "../src/DataManager.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <algorithm>

void printUsage(const char* programName) 
{
    std::cout << "Usage:\n"
              << "  Convert CSV to ZCD:\n"
              << "    " << programName << " convert <input.csv> <output.zcd>\n\n"
              << "  Read ZCD file:\n"
              << "    " << programName << " read <input.zcd> [count]\n"
              << "    count: number of records to display (default: 5)\n\n"
              << "  Display ZCD header:\n"
              << "    " << programName << " header <input.zcd>\n\n"
              << "  Verify CSV vs ZCD using DataManager (identicality test):\n"
              << "    " << programName << " verify <input.csv> <input.zcd>\n\n"
              << "  Search using index (no full scan):\n"
              << "    " << programName << " zcd-search <input.zcd> <zipcode_data.idx> <zip> [<zip> ...]\n\n"
              << "Examples:\n"
              << "  " << programName << " convert PT2_CSV.csv output.zcd\n"
              << "  " << programName << " read output.zcd 10\n"
              << "  " << programName << " header output.zcd\n"
              << "  " << programName << " verify PT2_CSV.csv output.zcd\n"
              << "  " << programName << " zcd-search output.zcd zipcode_data.idx 55455 30301\n";
              
}

bool convertCSVtoZCD(const std::string& inFile, const std::string& outFile) 
{
    CSVBuffer csvBuffer;
    
    HeaderRecord header;
    header.setFileStructureType("ZIPC");
    header.setVersion(1);
    header.setSizeFormatType(0);
    header.setBlockSize(512); // Placeholder
    header.setMinBlockSize(256); // Placeholder
    header.setIndexFileName("zipcode_data.idx");
    header.setIndexFileSchemaInfo("Primary Key: Zipcode"); // Placeholder. Binary defeats the purpose of this.
    header.setRecordCount(0);
    
    std::vector<FieldDef> fields;
    fields.push_back({"zipcode", 1});
    fields.push_back({"location", 3});
    fields.push_back({"state", 4});
    fields.push_back({"county", 3});
    fields.push_back({"latitude", 2});
    fields.push_back({"longitude", 2});
    
    header.setFields(fields);
    header.setFieldCount(csvBuffer.EXPECTED_FIELD_COUNT);
    header.setPrimaryKeyField(0);
    header.setAvailableListRBN(0); // Placeholder
    header.setSequenceSetListRBN(0); // Placeholder
    header.setStaleFlag(0);
    
    std::ofstream out(outFile, std::ios::binary);
    if (!out.is_open()) 
    {
        std::cerr << "Error: Cannot create output file: " << outFile << std::endl;
        return false;
    }
    
    auto headerData = header.serialize();
    header.setHeaderSize(headerData.size());  // ← FIX: Update header size
    
    std::cerr << "DEBUG: Serialized header size = " << headerData.size() << "\n";  // ← DEBUG HERE
    
    out.write(reinterpret_cast<char*>(headerData.data()), headerData.size());
    
    size_t recordCountOffset = 4 + 2 + 4 + 1 + 4 + 1 + 2 + 2 +
                                header.getIndexFileName().length() + header.getIndexFileSchemaInfo().length();
    
    if (!csvBuffer.openFile(inFile)) 
    {
        std::cerr << "Error: Failed to open CSV file: " << csvBuffer.getLastError() << std::endl;
        return false;
    }
    
    std::cout << "Converting " << inFile << " to " << outFile << "..." << std::endl;

    ZipCodeRecord record;
    while (csvBuffer.getNextRecord(record))
    {
        std::string recordStr = std::to_string(record.getZipCode()) + "," +
                               record.getLocationName() + "," +
                               std::string(record.getState()) + "," +
                               record.getCounty() + "," +
                               std::to_string(record.getLatitude()) + "," +
                               std::to_string(record.getLongitude());
        
        uint32_t len = recordStr.length();
        out.write(reinterpret_cast<char*>(&len), 4);
        out.write(recordStr.c_str(), len);
    }

    uint32_t actualRecordCount = csvBuffer.getRecordsProcessed();
    out.seekp(recordCountOffset);
    out.write(reinterpret_cast<char*>(&actualRecordCount), sizeof(uint32_t));
    
    csvBuffer.closeFile();
    out.close();
    
    std::cout << "Success: Converted " << actualRecordCount << " records" << std::endl;

    // Reopen for index creation
    std::cout << "Now generating index file..." << std::endl;
    if (!csvBuffer.openLengthIndicatedFile(outFile, header.getHeaderSize())) 
    {
        std::cerr << "Error: Failed to reopen file for indexing" << std::endl;
        return false;
    }

    PrimaryKeyIndex keyIndex;
    keyIndex.createFromDataFile(csvBuffer);
    csvBuffer.closeFile();

    if (keyIndex.write(header.getIndexFileName())) 
    {
        std::cout << "Success: Index file generated: " << header.getIndexFileName() << std::endl;
        
        std::fstream zcdFile(outFile, std::ios::binary | std::ios::in | std::ios::out);
        uint8_t validFlag = 1;
        size_t flagOffset = header.getHeaderSize() - 1;
        
        zcdFile.seekp(flagOffset);
        zcdFile.write(reinterpret_cast<char*>(&validFlag), sizeof(uint8_t));
        
        zcdFile.close();
    } 
    else 
    {
        std::cerr << "Error: Failed to write index file" << std::endl;
        return false;
    }
    return true;
}

bool convertCSVToBlockedSequenceSet(const std::string& csvFile, const std::string& zcbFile, 
                                    uint32_t blockSize = 1024, uint16_t minBlockSize = 256)
{
    CSVBuffer csvBuffer;
    if(!csvBuffer.openFile(csvFile))
    {
        std::cerr << "Failed to open CSV file." << std::endl;
        return false;
    }

    std::vector<ZipCodeRecord> allRecords;
    ZipCodeRecord record;
    while(csvBuffer.getNextRecord(record))
    {
        allRecords.push_back(record);
    }

    std::cout << "Read " << allRecords.size() << " records." << std::endl;

    std::sort(allRecords.begin(), allRecords.end(),
                [](const ZipCodeRecord& a, const ZipCodeRecord& b)
            {
                return a.getZipCode() < b.getZipCode();
            });

    std:: cout << "Sorted records by ZipCode." << std::endl;

    HeaderRecord header;

    header.setFileStructureType("ZIPC");
    header.setVersion(2);
    header.setHeaderSize(0); // Set In Serialization Process
    header.setSizeFormatType(0);
    header.setBlockSize(blockSize);
    header.setMinBlockSize(minBlockSize);
    header.setIndexFileName("zipcode_data.idx"); // Placeholder
    header.setIndexFileSchemaInfo("Primary Key: Zipcode"); // Placeholder
    header.setRecordCount(allRecords.size()); // This will need to be tracked and updated after if sorting changes
    header.setBlockCount(0); // Update After Conversion
    
    std::vector<FieldDef> fields;
    fields.push_back({"zipcode", 1});
    fields.push_back({"location", 3});
    fields.push_back({"state", 4});
    fields.push_back({"county", 3});
    fields.push_back({"latitude", 2});
    fields.push_back({"longitude", 2});
    
    header.setFields(fields);
    header.setFieldCount(csvBuffer.EXPECTED_FIELD_COUNT);
    header.setPrimaryKeyField(0);
    header.setAvailableListRBN(0); 
    header.setSequenceSetListRBN(1); 
    header.setStaleFlag(0);

    std::ofstream out(zcbFile, std::ios::binary);
    if (!out.is_open()) 
    {
        std::cerr << "Error: Cannot create output file: " << zcbFile << std::endl;
        return false;
    }
    
    auto headerData = header.serialize();
    header.setHeaderSize(headerData.size());
    
    out.write(reinterpret_cast<char*>(headerData.data()), headerData.size());
    
    size_t blockCountOffset = 4 + 2 + 4 + 1 + 4 + 2 + 2 + 
                         header.getIndexFileName().length() + 
                         2 + header.getIndexFileSchemaInfo().length() + 
                         sizeof(uint32_t);
    
    std::cout << "Converting " << csvFile << " to " << zcbFile << "..." << std::endl;

    RecordBuffer recordBuffer;
    BlockBuffer blockBuffer;

    if(!blockBuffer.openFile(zcbFile, header.getHeaderSize()))
    {
        std::cerr << "Failed to open block buffer." << std::endl;    
    }

    uint32_t currentRBN = 1;
    uint32_t blockCount = 0;
    std::vector<ZipCodeRecord> currentBlockRecords;
    size_t currentSize = 10;  // metadata

    for(const auto& rec : allRecords)
    {
        // Check if adding this record would overflow
        if (currentSize + rec.getRecordSize() + 4 > blockSize)
        {
            // Write current block
            ActiveBlock block;
            block.precedingRBN = (currentRBN == 1) ? 0 : currentRBN - 1;
            block.succeedingRBN = currentRBN + 1;  // Temp - will fix last block later
            block.recordCount = static_cast<uint16_t>(currentBlockRecords.size());
            
            recordBuffer.packBlock(currentBlockRecords, block.data, blockSize);
            blockBuffer.writeActiveBlockAtRBN(currentRBN, blockSize, header.getHeaderSize(), block);
            
            ++blockCount;
            ++currentRBN;
            currentBlockRecords.clear();
            currentSize = 10;  // Reset to metadata size
        }

        // Add record to current block
        currentBlockRecords.push_back(rec);
        currentSize += rec.getRecordSize() + 4;
    }

    // Write final block
    if (!currentBlockRecords.empty())
    {
        ActiveBlock block;
        block.precedingRBN = (currentRBN == 1) ? 0 : currentRBN - 1;
        block.succeedingRBN = 0;  // Last block
        block.recordCount = static_cast<uint16_t>(currentBlockRecords.size());
        
        recordBuffer.packBlock(currentBlockRecords, block.data, blockSize);
        blockBuffer.writeActiveBlockAtRBN(currentRBN, blockSize, header.getHeaderSize(), block);
        
        ++blockCount;
    }

    out.seekp(blockCountOffset);
    out.write(reinterpret_cast<char*>(&blockCount), sizeof(uint32_t));

    // Once index are setup creating the index and setting the stale flag will go here.

    csvBuffer.closeFile();
    blockBuffer.closeFile();

    return true;
}

bool readZCD(const std::string& inFile, int displayCount) 
{
    HeaderRecord header;
    HeaderBuffer headerBuffer;
    
    if (!headerBuffer.readHeader(inFile, header)) 
    {
        std::cerr << "Error: Failed to read header from " << inFile << std::endl;
        return false;
    }
    
    CSVBuffer buffer;
    if (!buffer.openLengthIndicatedFile(inFile, header.getHeaderSize())) 
    {
        std::cerr << "Error: Failed to open file: " << buffer.getLastError() << std::endl;
        return false;
    }
    
    std::cout << "Reading " << inFile << "\n";
    std::cout << "Displaying first " << displayCount << " records:\n" << std::endl;
    
    ZipCodeRecord record;
    int count = 0;
    while (count < displayCount && buffer.getNextLengthIndicatedRecord(record)) 
    {
        std::cout << "Record " << count << ": "
                  << "ZIP=" << record.getZipCode()
                  << ", Location=" << record.getLocationName()
                  << ", State=" << record.getState()
                  << ", County=" << record.getCounty()
                  << ", Lat=" << record.getLatitude()
                  << ", Lon=" << record.getLongitude() << std::endl;
        count++;
    }
    
    std::cout << "\nTotal records in file: " << header.getRecordCount() << std::endl;
    std::cout << "Records displayed: " << count << std::endl;
    
    buffer.closeFile();
    return true;
}

bool displayHeader(const std::string& inFile) 
{
    HeaderRecord header;
    HeaderBuffer headerBuffer;
    
    if (!headerBuffer.readHeader(inFile, header)) 
    {
        std::cerr << "Error: Failed to read header from " << inFile << std::endl;
        return false;
    }
    
    std::cout << "\n=== ZCD File Header Information ===\n";
    std::cout << "File: " << inFile << "\n";
    std::cout << "File Structure Type: " << std::string(header.getFileStructureType(), 4) << "\n";
    std::cout << "Version: " << header.getVersion() << "\n";
    std::cout << "Header Size: " << header.getHeaderSize() << " bytes\n";
    std::cout << "Size Format Type: " << (int)header.getSizeFormatType() << " (0=ASCII, 1=Binary)\n";
    std::cout << "Index File: " << header.getIndexFileName() << "\n";
    std::cout << "Has Valid Index: " << (header.getStaleFlag() ? "Yes" : "No") << "\n";
    std::cout << "Record Count: " << header.getRecordCount() << "\n";
    std::cout << "Field Count: " << header.getFieldCount() << "\n";
    std::cout << "Primary Key Field: " << (int)header.getPrimaryKeyField() << "\n";
    
    std::cout << "\nFields:\n";
    const auto& fields = header.getFields();
    for (size_t i = 0; i < fields.size(); i++) 
    {
        std::cout << "  [" << i << "] " << fields[i].name 
                  << " (type=" << (int)fields[i].type << ")\n";
    }
    
    return true;
}
static bool readLengthIndicatedRecordAt(const std::string& zcdPath,
                                        uint64_t absOffset,
                                        std::string& out)
{
    std::ifstream in(zcdPath, std::ios::binary);
    if (!in) return false;

    in.seekg(static_cast<std::streamoff>(absOffset), std::ios::beg);

    uint32_t len = 0;
    if (!in.read(reinterpret_cast<char*>(&len), 4)) return false;

    out.resize(len);
    return static_cast<bool>(in.read(out.data(), len));
}

int main(int argc, char* argv[]) 
{
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string command = argv[1];
    
    if (command == "convert") 
    {
        if (argc != 4) {
            std::cerr << "Error: convert requires input and output filenames\n";
            printUsage(argv[0]);
            return 1;
        }
        return convertCSVtoZCD(argv[2], argv[3]) ? 0 : 1;
    }
    else if (command == "read") 
    {
        if (argc < 3) {
            std::cerr << "Error: read requires input filename\n";
            printUsage(argv[0]);
            return 1;
        }
        int count = (argc >= 4) ? std::atoi(argv[3]) : 5;
        return readZCD(argv[2], count) ? 0 : 1;
    }
    else if (command == "header") 
    {
        if (argc != 3) {
            std::cerr << "Error: header requires input filename\n";
            printUsage(argv[0]);
            return 1;
        }
        return displayHeader(argv[2]) ? 0 : 1;
    }
    else if (command == "verify") 
    {
    if (argc != 4) {
        printUsage(argv[0]);
        return 1;
    }
    // CSV is NOT length-indicated; ZCD IS length-indicated
    bool ok = DataManager::verifyIdenticalResults(
                  argv[2],              // CSV path
                  argv[3],              // ZCD path
                  /*fileAIsLengthIndicated=*/false,
                  /*fileBIsLengthIndicated=*/true
              );
    std::cout << (ok ? "IDENTICAL\n" : "DIFFER\n");
    return ok ? 0 : 2;
    }
    else if (command == "zcd-search") {
    if (argc < 5) { printUsage(argv[0]); return 1; }

    std::string zcdPath = argv[2];
    std::string idxPath = argv[3];

    // sanity-check header
    HeaderRecord hdr; HeaderBuffer hb;
    if (!hb.readHeader(zcdPath, hdr)) {
        std::cerr << "Failed to read header for " << zcdPath << "\n";
        return 1;
    }

    // Load index
    PrimaryKeyIndex idx;
    if (!idx.read(idxPath)) {
        std::cerr << "Failed to load index " << idxPath << "\n";
        return 1;
    }

    // Handle one or more ZIPs on the command line
    for (int i = 4; i < argc; ++i) {
        uint32_t zip = static_cast<uint32_t>(std::stoul(argv[i]));
        auto offsets = idx.find(zip);

        if (offsets.empty()) {
            std::cout << zip << ": NOT FOUND\n";
            continue;
        }

        for (size_t off : offsets) {
            std::string rec;
            if (!readLengthIndicatedRecordAt(zcdPath, static_cast<uint64_t>(off), rec)) {
                std::cout << zip << ": BAD OFFSET " << off << "\n";
                continue;
            }
            // rec is "zip,location,state,county,lat,lon"
            std::cout << zip << ": " << rec << "\n";
        }
    }
    return 0;
    }

    else 
    {
        std::cerr << "Error: Unknown command '" << command << "'\n";
        printUsage(argv[0]);
        return 1;
    }
}