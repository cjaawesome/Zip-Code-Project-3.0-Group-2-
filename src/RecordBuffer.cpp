#include "RecordBuffer.h"

bool RecordBuffer::unpackBlock(const std::vector<char>& blockData, std::vector<ZipCodeRecord>& records){
    records.clear();

    if (blockData.empty()) return false;

    //finish unpackBlock
}

bool RecordBuffer::packBlock(const std::vector<ZipCodeRecord>& records, std::vector<char>& blockData){
    blockData.clear();

    if (records.empty()) return false;

    //finish packBlock
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