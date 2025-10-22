/**
 * @file ZipCodeRecord.cpp
 * @author Group 2
 * @brief Implementation of ZipCodeRecord class
 * @version 0.1
 * @date 2025-09-17
 */

#include "ZipCodeRecord.h"
#include <cstring>
#include <iostream>
#include <iomanip>

/**
 * @brief Default constructor
 * @details Initializes all fields to default values
 */
ZipCodeRecord::ZipCodeRecord() 
    : zipCode(0), latitude(0.0), longitude(0.0), locationName(""), county("")
{
    state[0] = '\0';  // Initialize state as empty string
}

/**
 * @brief Parameterized constructor
 * @details Creates record with all specified values, validates coordinates
 */
ZipCodeRecord::ZipCodeRecord(const int inZipCode, const double inLatitude, 
                            const double inLongitude, const std::string& inLocationName, 
                            const std::string& inState, const std::string& inCounty)
    : zipCode(0), latitude(0.0), longitude(0.0), locationName(""), county("")
{
    state[0] = '\0';
    
    // Setter methods for validation
    setZipCode(inZipCode);
    setLatitude(inLatitude);
    setLongitude(inLongitude);
    setLocationName(inLocationName);
    setState(inState);
    setCounty(inCounty);
}

/**
 * @brief Copy constructor
 * @details Creates copy of another ZipCodeRecord
 */
ZipCodeRecord::ZipCodeRecord(const ZipCodeRecord& other)
    : zipCode(other.zipCode), latitude(other.latitude), longitude(other.longitude),
      locationName(other.locationName), county(other.county)
{
    strcpy(state, other.state);
}

/**
 * @brief Assignment operator
 * @details Assigns all fields from another record
 */
ZipCodeRecord& ZipCodeRecord::operator=(const ZipCodeRecord& other)
{
    if (this != &other) 
    {  // Prevent self-assignment
        zipCode = other.zipCode;
        latitude = other.latitude;
        longitude = other.longitude;
        locationName = other.locationName;
        county = other.county;
        strcpy(state, other.state);
    }
    return *this;
}

/**
 * @brief Destructor
 * @details Clean up 
 */
ZipCodeRecord::~ZipCodeRecord()
{
}

// Setter implementations
bool ZipCodeRecord::setZipCode(const uint32_t inZipCode)
{
    if (inZipCode > 0 && inZipCode <= 99999) // Valid US zip code range
    {  
        zipCode = inZipCode;
        return true;
    }
    return false;
}

bool ZipCodeRecord::setLatitude(const double inLatitude)
{
    if (inLatitude >= -90.0 && inLatitude <= 90.0) 
    {
        latitude = inLatitude;
        return true;
    }
    return false;
}

bool ZipCodeRecord::setLongitude(const double inLongitude)
{
    if (inLongitude >= -180.0 && inLongitude <= 180.0) 
    {
        longitude = inLongitude;
        return true;
    }
    return false;
}

bool ZipCodeRecord::setLocationName(const std::string& inLocationName)
{
    if (!inLocationName.empty() && inLocationName.length() < 100) 
    {  
        locationName = inLocationName;
        return true;
    }
    return false;
}

bool ZipCodeRecord::setState(const std::string& inState)
{
    if (inState.length() == 2) // Must be exactly 2 characters
    {  
        state[0] = inState[0];
        state[1] = inState[1];
        state[2] = '\0';
        return true;
    }
    return false;
}

bool ZipCodeRecord::setCounty(const std::string& inCounty)
{
    if (!inCounty.empty() && inCounty.length() < 50) 
    {  
        county = inCounty;
        return true;
    }
    return false;
}

// Getter implementations
uint32_t ZipCodeRecord::getZipCode() const
{
    return zipCode;
}

double ZipCodeRecord::getLatitude() const
{
    return latitude;
}

double ZipCodeRecord::getLongitude() const
{
    return longitude;
}

std::string ZipCodeRecord::getLocationName() const
{
    return locationName;
}

const char* ZipCodeRecord::getState() const
{
    return state;
}

std::string ZipCodeRecord::getCounty() const
{
    return county;
}

// Comparison methods for determining extremes
bool ZipCodeRecord::isNorthOf(const ZipCodeRecord& other) const
{
    return latitude > other.latitude;
}

bool ZipCodeRecord::isEastOf(const ZipCodeRecord& other) const
{
    return longitude > other.longitude; // East means larger longitude (less negative/more positive in US)
}

bool ZipCodeRecord::isSouthOf(const ZipCodeRecord& other) const
{
    return latitude < other.latitude;
}

bool ZipCodeRecord::isWestOf(const ZipCodeRecord& other) const
{
    return longitude < other.longitude; // West means smaller longitude (more negative in US)
}

// Stream output operator
std::ostream& operator<<(std::ostream& outputStream, const ZipCodeRecord& record)
{
    outputStream << std::fixed << std::setprecision(4)
       << "Zip: " << std::setw(5) << std::setfill('0') << record.zipCode
       << ", " << record.locationName
       << ", " << record.state 
       << ", " << record.county
       << " (" << record.latitude << ", " << record.longitude << ")";
    return outputStream;
}

 std::vector<uint8_t> ZipCodeRecord::serialize() const
 {
    std::vector<uint8_t> data; // Stores the binary data

    // Zip Code
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&zipCode),
                reinterpret_cast<const uint8_t*>(&zipCode) + sizeof(zipCode));

    // Location Name Length
    uint16_t locationNameLength = locationName.length();
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&locationNameLength),
                reinterpret_cast<const uint8_t*>(&locationNameLength) + sizeof(locationNameLength));
    // Location Name
    data.insert(data.end(), locationName.begin(), locationName.end());
    
    // County Name Length
    uint16_t countyLength = county.length();
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&countyLength),
                reinterpret_cast<const uint8_t*>(&countyLength) + sizeof(countyLength));
    // County Name
    data.insert(data.end(), county.begin(), county.end());

    // State
    data.insert(data.end(), state, state + 3);

    // Latitude
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&latitude),
                reinterpret_cast<const uint8_t*>(&latitude) + sizeof(latitude));

    // Longitude
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&longitude),
                reinterpret_cast<const uint8_t*>(&longitude) + sizeof(longitude));

    return data;
 }

 ZipCodeRecord ZipCodeRecord::deserialize(const uint8_t* data, size_t length)
 {
    ZipCodeRecord record;
    size_t offset = 0;

    // Read Zipcode
    memcpy(&record.zipCode, data + offset, sizeof(int));
    offset += sizeof(int);

    // Read Location Name Length
    uint16_t locationNameLength;
    memcpy(&locationNameLength, data + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    // Read Location Name
    record.locationName = std::string(reinterpret_cast<const char*>(data + offset), locationNameLength);
    offset += locationNameLength;

    // Read County Name Length
    uint16_t countyNameLength;
    memcpy(&countyNameLength, data + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    // Read County Name
    record.county = std::string(reinterpret_cast<const char*>(data + offset), countyNameLength);
    offset += countyNameLength;

    // Read State
    memcpy(record.state, data + offset, 3);
    offset += 3;

    // Read Latitude
    memcpy(&record.latitude, data + offset, sizeof(double));
    offset += sizeof(double);

    // Read Longitude
    memcpy(&record.longitude, data + offset, sizeof(double));
    offset += sizeof(double);

    return record;
 }