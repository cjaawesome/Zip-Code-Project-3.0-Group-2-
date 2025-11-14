#include "RecordBuffer.h"
#include "ZipCodeRecord.h"
#include <cstring>


RecordBuffer::RecordBuffer(){
    // :)
}


RecordBuffer::~RecordBuffer(){

}

bool RecordBuffer::unpackBlock(const std::vector<char>& blockData, std::vector<ZipCodeRecord>& records)
{
    records.clear();
    if (blockData.empty()) return false;

    size_t offset = 0;
    int recordNum = 0;

    while(offset + 4 <= blockData.size())
    {
        uint32_t lengthPrefix;
        std::memcpy(&lengthPrefix, &blockData[offset], sizeof(uint32_t));
        
        if (blockData[offset] == '\xFF') 
        {
            //std::cout << "  Hit padding at offset " << offset << "\n";
            break;
        }
        
        offset += 4;

        if (lengthPrefix == 0 || offset + lengthPrefix > blockData.size())
        {
            std::cout << "  Breaking: invalid length or overflow\n";
            break;
        }

        std::string recordStr(blockData.begin() + offset, blockData.begin() + offset + lengthPrefix);
        offset += lengthPrefix;
        
        ZipCodeRecord record;
        if(!parseZipCodeRecord(recordStr, record)) 
        {
            std::cout << "  Parse failed!\n";
            setError("Error Parsing ZipCodeRecord within Unpack Block. Block Skipped.");
            return false;
        }
        records.push_back(record);
        recordNum++;
    }
    return true;
}

bool RecordBuffer::packBlock(const std::vector<ZipCodeRecord>& records, std::vector<char>& blockData, const uint32_t blockSize)
{
    blockData.clear();
    if (records.empty()) return false;

    blockData.reserve(blockSize);
    for(const auto& record : records)
    {
        size_t oldSize = blockData.size();
        blockData.resize(oldSize + sizeof(uint32_t));

        std::string recordStr = std::to_string(record.getZipCode()) + "," +
                               record.getLocationName() + "," +
                               std::string(record.getState()) + "," +
                               record.getCounty() + "," +
                               std::to_string(record.getLatitude()) + "," +
                               std::to_string(record.getLongitude());

        uint32_t lengthPrefix = recordStr.length();

        std::memcpy(&blockData[oldSize], &lengthPrefix, sizeof(uint32_t));
        blockData.insert(blockData.end(), recordStr.begin(), recordStr.end());

        size_t totalBlockSize = sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t) + 
                                sizeof(uint32_t) + blockData.size() + lengthPrefix;            

        if (totalBlockSize > blockSize) 
        {
           setError("Block size exceeded during packing");
           return false;
        }
    }
    return true;
}

bool RecordBuffer::parseZipCodeRecord(const std::string& recordStr, ZipCodeRecord& record)
{
    std::vector<std::string> fields;
    std::stringstream ss(recordStr);
    std::string field;

    while(std::getline(ss, field, ','))
    {
        trimString(field);
        fields.push_back(field);
    }
    return fieldsToRecord(fields, record);
}

bool RecordBuffer::fieldsToRecord(const std::vector<std::string>& fields, ZipCodeRecord& record)
{
    if (fields.size() != EXPECTED_FIELD_COUNT) 
        return false;
    
    // Validate and convert each field
    // Field 0: Zip Code (integer)
    if (!isValidUInt32(fields[0])) 
    {
        return false;
    }
    uint32_t zipCode = static_cast<uint32_t>(std::stoul(fields[0]));
    
    // Field 4: Latitude (double)
    if (!isValidDouble(fields[4])) 
    {
        return false;
    }
    double latitude = std::stod(fields[4]);
    
    // Field 5: Longitude (double)  
    if (!isValidDouble(fields[5])) 
    {
        return false;
    }
    double longitude = std::stod(fields[5]);
    
    // Field 1: Place Name (string)
    std::string placeName = fields[1];
    
    // Field 2: State (string, must be 2 chars)
    std::string state = fields[2];
    if (state.length() != 2)
    {
        return false;
    }
    
    // Field 3: County (string)
    std::string county = fields[3];
    
    // Create the record
    ZipCodeRecord tempRecord(zipCode, latitude, longitude, placeName, state, county);
    record = tempRecord;
    
    return true;
}

bool RecordBuffer::isValidUInt32(const std::string& str) const
{
    if (str.empty()) return false;
    
    try 
    {
        long val = std::stol(str);
        if (val < 0 || val > 4294967295) return false;
        return true;
    } 
    catch (const std::exception&) 
    {
        return false;
    }
}

bool RecordBuffer::isValidDouble(const std::string& str) const
{
    if (str.empty()) return false;
    
    try 
    {
        std::stod(str);
        return true;
    } 
    catch (const std::exception&) 
    {
        return false;
    }
}

bool RecordBuffer::hasError() const
{
    return errorState;
}

void RecordBuffer::setError(const std::string& message)
{
    errorState = true;
    lastError = message;
}

void RecordBuffer::trimString(std::string& str)
{
    // Remove leading whitespace
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) 
    {
        return !std::isspace(ch);
    }));
    
    // Remove trailing whitespace  
    str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) 
    {
        return !std::isspace(ch);
    }).base(), str.end());
}

std::string RecordBuffer::getLastError() const
{
    return lastError;
}