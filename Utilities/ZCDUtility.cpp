// ZCDUtility.cpp
#include "../src/HeaderRecord.h"
#include "../src/HeaderBuffer.h"
#include "../src/CSVBuffer.h"
#include "../src/ZipCodeRecord.h"
#include "../src/PrimaryKeyIndex.h"
#include "../src/BlockBuffer.h"
#include "../src/DataManager.h"
#include "../src/BlockIndexFile.h"
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
              << "  Convert CSV to Blocked Sequence Set:\n"
              << "    " << programName << " convert-blocked <input.csv> <output.zcb> [blockSize] [minBlockSize]\n"
              << "    blockSize: block size in bytes (default: 1024)\n"
              << "    minBlockSize: minimum block size (default: 256)\n\n"
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
              << "  " << programName << " convert-blocked PT2_CSV.csv output.zcb\n"
              << "  " << programName << " convert-blocked PT2_CSV.csv output.zcb 2048 512\n"
              << "  " << programName << " read output.zcd 10\n"
              << "  " << programName << " header output.zcd\n"
              << "  " << programName << " verify PT2_CSV.csv output.zcd\n"
              << "  " << programName << " zcd-search output.zcd zipcode_data.idx 55455 30301\n";

}

bool convertCSVtoZCD(const std::string& inFile, const std::string& outFile) 
{
    CSVBuffer csvBuffer;

    HeaderRecord header;
    header.setFileStructureType("ZCD");      // <- use consistent type
    header.setVersion(1);
    header.setSizeFormatType(0);             // whatever your binary format enum means
    header.setBlockSize(0);                  // ZCD: not used
    header.setMinBlockSize(0);               // ZCD: not used
    header.setIndexFileName("zipcode_data.idx");
    header.setIndexFileSchemaInfo("Primary Key: Zipcode");
    header.setRecordCount(0);
    header.setAvailableListRBN(-1);          // ZCD: not used
    header.setSequenceSetListRBN(-1);        // ZCD: not used
    header.setStaleFlag(true);               // becomes false after index is written

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

    // open output file
    std::ofstream out(outFile, std::ios::binary | std::ios::trunc);
    if (!out.is_open()){
        std::cerr << "Error: Cannot create output file: " << outFile << std::endl;
        return false;
    }

    // --- write a temporary header; we will rewrite it at the end with final sizes ---
    {
        auto headerData = header.serialize();
        header.setHeaderSize(static_cast<uint32_t>(headerData.size())); // set it
        headerData = header.serialize(); // re-serialize so headerSize is embedded correctly
        std::cerr << "DEBUG: Serialized header size = " << headerData.size() << "\n";
        out.write(reinterpret_cast<const char*>(headerData.data()), headerData.size());
    }

    if (!csvBuffer.openFile(inFile)){
        std::cerr << "Error: Failed to open CSV file: " << csvBuffer.getLastError() << std::endl;
        return false;
    }

    std::cout << "Converting " << inFile << " to " << outFile << "..." << std::endl;

    // write length-indicated, comma-separated records
    ZipCodeRecord record;
    uint64_t count = 0;

    // helpers for stable float formatting
    auto to_fixed = [](double v){
        std::ostringstream oss;
        oss.setf(std::ios::fixed); 
        oss.precision(6);
        oss << v;
        return oss.str();
    };

    while (csvBuffer.getNextRecord(record)){
        std::string recordStr =
            std::to_string(record.getZipCode()) + "," +
            record.getLocationName() + "," +
            std::string(record.getState()) + "," +
            record.getCounty() + "," +
            to_fixed(record.getLatitude()) + "," +
            to_fixed(record.getLongitude());

        uint32_t len = static_cast<uint32_t>(recordStr.size());
        out.write(reinterpret_cast<const char*>(&len), sizeof(uint32_t));
        out.write(recordStr.data(), len);
        ++count;
    }
    csvBuffer.closeFile();
    out.close();

    std::cout << "Success: Converted " << count << " records" << std::endl;

    // --- Generate simple index from the ZCD we just wrote ---
    std::cout << "Now generating index file..." << std::endl;

    if (!csvBuffer.openLengthIndicatedFile(outFile, header.getHeaderSize())){
        std::cerr << "Error: Failed to reopen file for indexing" << std::endl;
        return false;
    }
    PrimaryKeyIndex keyIndex;
    keyIndex.createFromDataFile(csvBuffer);
    csvBuffer.closeFile();

    if (!keyIndex.write(header.getIndexFileName())){
        std::cerr << "Error: Failed to write index file" << std::endl;
        return false;
    }
    std::cout << "Success: Index file generated: " << header.getIndexFileName() << std::endl;

    // --- Rewrite header at file start with final values (recordCount, headerSize, staleFlag=false) ---
    header.setRecordCount(count);
    header.setStaleFlag(false);

    // use your HeaderBuffer if you have one; otherwise rewrite manually
    {
        std::fstream io(outFile, std::ios::binary | std::ios::in | std::ios::out);
        if (!io){
            std::cerr << "Warning: could not reopen ZCD to rewrite header\n";
        } else {
            auto finalHeader = header.serialize();
            header.setHeaderSize(static_cast<uint32_t>(finalHeader.size()));
            finalHeader = header.serialize(); // re-serialize with updated size again
            io.seekp(0, std::ios::beg);
            io.write(reinterpret_cast<const char*>(finalHeader.data()), finalHeader.size());
            io.flush();
        }
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
    header.setIndexFileName("data/zipcode_data.idx"); // Placeholder
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

    csvBuffer.closeFile();
    blockBuffer.closeFile();
    out.close();

    // Once index are setup creating the index and setting the stale flag will go here.

    BlockIndexFile index;
    if(index.createIndexFromBlockedFile(zcbFile, blockSize, header.getHeaderSize(), 
                                        header.getSequenceSetListRBN()))
    {
        std::cout << "Index Succesfully Created. Now Writing Index." << std::endl;
        if(index.write(header.getIndexFileName()))
        {
            std::cout << "Index Successfully Written" << std::endl;
            std::fstream outFile(zcbFile, std::ios::binary | std::ios::in | std::ios::out);
            uint8_t staleFlag = 0;
            size_t flagOffset = header.getHeaderSize() - 1;
            
            outFile.seekp(flagOffset);
            outFile.write(reinterpret_cast<char*>(&staleFlag), sizeof(uint8_t));
            
            outFile.close();
        }
        else
        {
            std::cerr << "Error: Failed to write index file" << std::endl;
            return false;
        }
    }

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

// quick CSV parser for a single line "zip,location,state,county,lat,lon"
static bool parseOneCSVLine(const std::string& line, ZipCodeRecord& rec) {
    std::stringstream ss(line);
    std::string zip, location, state, county, lat, lon;

    if (!std::getline(ss, zip, ',')) return false;
    if (!std::getline(ss, location, ',')) return false;
    if (!std::getline(ss, state, ',')) return false;
    if (!std::getline(ss, county, ',')) return false;
    if (!std::getline(ss, lat, ',')) return false;
    if (!std::getline(ss, lon, ',')) return false;

    try {
        rec.setZipCode(static_cast<uint32_t>(std::stoul(zip)));
        rec.setLocationName(location);
        rec.setState(state.c_str());
        rec.setCounty(county);
        rec.setLatitude(std::stod(lat));
        rec.setLongitude(std::stod(lon));
        return true;
    } catch (...) { return false; }
}

// get highest zip in a block (assumes block.data holds sorted records)
static uint32_t highestZipInBlock(BlockBuffer& bb,
                                  uint32_t rbn,
                                  uint32_t blockSize,
                                  size_t headerSize,
                                  RecordBuffer& rb)
{
    auto blk = bb.loadActiveBlockAtRBN(rbn, blockSize, headerSize);
    std::vector<ZipCodeRecord> recs;
    rb.unpackBlock(blk.data, recs);
    if (recs.empty()) return 0;
    return recs.back().getZipCode();
}

// follow logical chain to find target block RBN for a zip
static uint32_t findTargetBlockRBN(BlockBuffer& bb,
                                   uint32_t seqHead,
                                   uint32_t zip,
                                   uint32_t blockSize,
                                   size_t headerSize,
                                   RecordBuffer& rb)
{
    uint32_t curr = seqHead;
    while (curr != 0) {
        auto blk = bb.loadActiveBlockAtRBN(curr, blockSize, headerSize);
        std::vector<ZipCodeRecord> recs; rb.unpackBlock(blk.data, recs);
        if (!recs.empty()) {
            uint32_t highest = recs.back().getZipCode();
            if (zip <= highest) return curr;  // fits here
        }
        curr = blk.succeedingRBN;
    }
    // larger than all -> insert in rightmost block (the last in chain)
    // find rightmost:
    curr = seqHead;
    uint32_t last = 0;
    while (curr != 0) {
        auto blk = bb.loadActiveBlockAtRBN(curr, blockSize, headerSize);
        last = curr;
        curr = blk.succeedingRBN;
    }
    return last;
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
    else if (command == "convert-blocked")
    {
        if (argc < 4) {
            std::cerr << "Error: convert-blocked requires input and output filenames\n";
            printUsage(argv[0]);
            return 1;
        }
        uint32_t blockSize = (argc >= 5) ? std::atoi(argv[4]) : 1024;
        uint16_t minBlockSize = (argc >= 6) ? std::atoi(argv[5]) : 256;
        return convertCSVToBlockedSequenceSet(argv[2], argv[3], blockSize, minBlockSize) ? 0 : 1;
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
    else if (command == "dump-physical")
{
    if (argc != 3) {
        std::cerr << "Error: dump-physical requires <blocked.zcb>\n";
        printUsage(argv[0]);
        return 1;
    }
    const std::string zcb = argv[2];

    HeaderRecord hdr; HeaderBuffer hb;
    if (!hb.readHeader(zcb, hdr)) {
        std::cerr << "Error: bad or missing header in " << zcb << "\n";
        return 1;
    }

    BlockBuffer bb;
    if (!bb.openFile(zcb, hdr.getHeaderSize())) {
        std::cerr << "Error: cannot open " << zcb << "\n";
        return 1;
    }

    bb.dumpPhysicalOrder(std::cout,
                         hdr.getSequenceSetListRBN(),  // head of active list
                         hdr.getAvailableListRBN(),    // head of avail list
                         hdr.getBlockCount(),          // number of blocks
                         hdr.getBlockSize(),
                         hdr.getHeaderSize());
    return 0;
}
else if (command == "dump-logical")
{
    if (argc != 3) {
        std::cerr << "Error: dump-logical requires <blocked.zcb>\n";
        printUsage(argv[0]);
        return 1;
    }
    const std::string zcb = argv[2];

    HeaderRecord hdr; HeaderBuffer hb;
    if (!hb.readHeader(zcb, hdr)) {
        std::cerr << "Error: bad or missing header in " << zcb << "\n";
        return 1;
    }

    BlockBuffer bb;
    if (!bb.openFile(zcb, hdr.getHeaderSize())) {
        std::cerr << "Error: cannot open " << zcb << "\n";
        return 1;
    }

    bb.dumpLogicalOrder(std::cout,
                        hdr.getSequenceSetListRBN(),  // head of active list
                        hdr.getAvailableListRBN(),    // head of avail list
                        hdr.getBlockSize(),
                        hdr.getHeaderSize());
    return 0;
}
else if (command == "add")
{
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " add <blocked.zcb> <records.csv>\n";
        return 1;
    }
    const std::string zcb = argv[2];
    const std::string recFile = argv[3];

    HeaderRecord hdr; HeaderBuffer hb;
    if (!hb.readHeader(zcb, hdr)) {
        std::cerr << "Error: bad header in " << zcb << "\n";
        return 1;
    }

    BlockBuffer bb;
    if (!bb.openFile(zcb, hdr.getHeaderSize())) {
        std::cerr << "Error: cannot open " << zcb << "\n";
        return 1;
    }

    std::ifstream in(recFile);
    if (!in) { std::cerr << "Error: cannot open " << recFile << "\n"; return 1; }

    RecordBuffer rb;
    uint32_t avail = static_cast<uint32_t>(hdr.getAvailableListRBN());
    uint32_t blocks = hdr.getBlockCount();
    uint32_t seqHead = hdr.getSequenceSetListRBN();
    if (seqHead == 0) { std::cerr << "Error: empty sequence set.\n"; return 1; }

    size_t added = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        ZipCodeRecord rec;
        if (!parseOneCSVLine(line, rec)) {
            std::cerr << "Skip bad line: " << line << "\n"; continue;
        }
        uint32_t target = findTargetBlockRBN(bb, seqHead, rec.getZipCode(),
                                             hdr.getBlockSize(), hdr.getHeaderSize(), rb);

        uint32_t blocksBefore = blocks;
        uint32_t availBefore  = avail;

        if (!bb.addRecord(target, hdr.getBlockSize(), avail, rec, hdr.getHeaderSize(), blocks)) {
            std::cerr << "ADD failed for zip " << rec.getZipCode() << "\n";
            continue;
        }
        ++added;

        // Log split if block count changed or avail head consumed
        if (blocks != blocksBefore || avail != availBefore) {
            std::cout << "SPLIT: added zip " << rec.getZipCode()
                      << " (blocks " << blocksBefore << "→" << blocks
                      << ", availHead " << availBefore << "→" << avail << ")\n";
        }
    }

    // persist header
    hdr.setAvailableListRBN(static_cast<int32_t>(avail));
    hdr.setBlockCount(blocks);
    hb.writeHeader(zcb, hdr);

    std::cout << "ADD: inserted " << added << " records.\n";
    return 0;
}
else if (command == "del")
{
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " del <blocked.zcb> <keys.txt>\n";
        return 1;
    }
    const std::string zcb = argv[2];
    const std::string keyFile = argv[3];

    HeaderRecord hdr; HeaderBuffer hb;
    if (!hb.readHeader(zcb, hdr)) {
        std::cerr << "Error: bad header in " << zcb << "\n";
        return 1;
    }

    BlockBuffer bb;
    if (!bb.openFile(zcb, hdr.getHeaderSize())) {
        std::cerr << "Error: cannot open " << zcb << "\n";
        return 1;
    }

    std::ifstream in(keyFile);
    if (!in) { std::cerr << "Error: cannot open " << keyFile << "\n"; return 1; }

    RecordBuffer rb;
    uint32_t avail = static_cast<uint32_t>(hdr.getAvailableListRBN());
    uint32_t blocks = hdr.getBlockCount();
    uint32_t seqHead = hdr.getSequenceSetListRBN();
    if (seqHead == 0) { std::cerr << "Error: empty sequence set.\n"; return 1; }

    size_t removed = 0;
    std::string s;
    while (std::getline(in, s)) {
        if (s.empty()) continue;
        uint32_t zip = 0;
        try { zip = static_cast<uint32_t>(std::stoul(s)); } catch (...) { continue; }

        // find candidate block by walking until highest >= zip
        uint32_t target = findTargetBlockRBN(bb, seqHead, zip,
                                             hdr.getBlockSize(), hdr.getHeaderSize(), rb);
        uint32_t availBefore  = avail;
        uint32_t blocksBefore = blocks;

        bool ok = bb.removeRecordAtRBN(target,
                                       static_cast<uint16_t>(hdr.getMinBlockSize()),
                                       avail, zip, hdr.getBlockSize(), hdr.getHeaderSize());
        if (!ok) {
            std::cout << "DELETE: zip " << zip << " not found (or unchanged)\n";
            continue;
        }
        ++removed;

        // Log merge/redistribution heuristic (we don’t have explicit flags; rely on counters)
        if (blocks != blocksBefore || avail != availBefore) {
            std::cout << "MERGE/REDIST: removed zip " << zip
                      << " (blocks " << blocksBefore << "→" << blocks
                      << ", availHead " << availBefore << "→" << avail << ")\n";
        }
    }

    // persist header
    hdr.setAvailableListRBN(static_cast<int32_t>(avail));
    hdr.setBlockCount(blocks);
    hb.writeHeader(zcb, hdr);

    std::cout << "DEL: removed " << removed << " keys.\n";
    return 0;
}
else if (command == "dump-block" && argc == 4) {
    const std::string zcb = argv[2];
    const uint32_t rbn = static_cast<uint32_t>(std::stoul(argv[3]));

    HeaderRecord hdr; HeaderBuffer hb;
    if (!hb.readHeader(zcb, hdr)) { std::cerr << "Bad header\n"; return 1; }

    BlockBuffer bb;
    if (!bb.openFile(zcb, hdr.getHeaderSize())) { std::cerr << "Open failed\n"; return 1; }

    auto blk = bb.loadActiveBlockAtRBN(rbn, hdr.getBlockSize(), hdr.getHeaderSize());
    std::cout << "RBN " << rbn << " recordCount=" << blk.recordCount
              << " prev=" << blk.precedingRBN << " next=" << blk.succeedingRBN << "\n";

    if (blk.recordCount == 0) {
        auto ab = bb.loadAvailBlockAtRBN(rbn, hdr.getBlockSize(), hdr.getHeaderSize());
        std::cout << "  *available*  nextAvail=" << ab.succeedingRBN << "\n";
    } else {
        std::vector<ZipCodeRecord> recs;
        RecordBuffer rb;
        rb.unpackBlock(blk.data, recs);
        std::cout << "  zips: ";
        for (auto &r : recs) std::cout << r.getZipCode() << " ";
        std::cout << "\n";
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