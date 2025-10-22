#ifndef ZIP_CODE_RECORD_H
#define ZIP_CODE_RECORD_H

#include <string>
#include <iostream>
#include <vector>
#include "stdint.h"

/**
 * @file ZipCodeRecord.h
 * @author Group 2
 * @brief ZipCodeRecord class for storing individual zip code information
 * @version 0.1
 * @date 2025-09-11
 */

/**
 * @class ZipCodeRecord
 * @brief Represents a single zip code record with geographic data
 * @details Stores zip code, coordinates, and location information.
 */
class ZipCodeRecord
{
public:
    /**
     * @brief Default constructor
     * @details Initializes all fields to default values
     */
    ZipCodeRecord();
    
    /**
     * @brief Parameterized constructor
     * @param inZipCode [IN] 5-digit zip code
     * @param inLatitude [IN] Latitude coordinate (-90 to 90)
     * @param inLongitude [IN] Longitude coordinate (-180 to 180)
     * @param inLocationName [IN] Place name
     * @param inState [IN] Two-character state code
     * @param inCounty [IN] County name
     * @pre All parameters must be valid (zip code > 0, coordinates in range)
     * @post Object is initialized with provided values
     */
    ZipCodeRecord(const int inZipCode, const double inLatitude, const double inLongitude, 
                  const std::string& inLocationName, const std::string& inState, 
                  const std::string& inCounty);
    
    /**
     * @brief Copy constructor
     * @param other [IN] ZipCodeRecord to copy
     */
    ZipCodeRecord(const ZipCodeRecord& other);
    
    /**
     * @brief Assignment operator
     * @param other [IN] ZipCodeRecord to assign from
     * @return Reference to this object
     */
    ZipCodeRecord& operator=(const ZipCodeRecord& other);
    
    /**
     * @brief Destructor
     */
    ~ZipCodeRecord();

    // Setters
    /**
     * @brief Set zip code value
     * @param inZipCode [IN] 5-digit zip code
     * @return true if valid zip code, false otherwise
     * @pre inZipCode must be positive
     * @post zipCode is updated if valid
     */
    bool setZipCode(const uint32_t inZipCode);
    /**
     * @brief Set Latitude code value
     * @param inLatitude [DOUB] new latitude value
     * @return true if valid latitute, false otherwise
     * @pre inLatitude must be in the range -90.0-90.0
     * @post latitude is updated if valid
     */
    bool setLatitude(const double inLatitude);
    /**
     * @brief Set Longitude code value
     * @param inLongitude [DOUB] new latitude value
     * @return true if valid Longitude, false otherwise
     * @pre inLongitude must be in the range -180.0-180.0
     * @post longitude is updated if valid
     */
    bool setLongitude(const double inLongitude);
    /**
     * @brief Set Location name value
     * @param inLocationName [STR] new location name
     * @return true if valid location name, false otherwise
     * @pre inLocationName must have a length less than 100 and not be empty
     * @post locationName is updated if valid
     */
    bool setLocationName(const std::string& inLocationName);
    /**
     * @brief Set state code value
     * @param inState [STR] new state code
     * @return true if valid state code, false otherwise
     * @pre inState must have a length is 2
     * @post state is updated if valid
     */
    bool setState(const std::string& inState);
    /**
     * @brief Set county name value
     * @param inCounty [STR] new county name
     * @return true if valid county name, false otherwise
     * @pre inCounty must have a length less than 50 and not be empty
     * @post county is updated if valid
     */
    bool setCounty(const std::string& inCounty);

    // Getters
    /**
     * @brief Zipcode Getter
     * @return zipcode
     */
    uint32_t getZipCode() const;
    /**
     * @brief Latitude Getter
     * @return latitude
     */
    double getLatitude() const;
    /**
     * @brief Longitude Getter
     * @return longitude
     */
    double getLongitude() const;
    /**
     * @brief Location Name Getter
     * @return locationName
     */
    std::string getLocationName() const;
    /**
     * @brief State Code Getter
     * @return state
     */
    const char* getState() const;
    /**
     * @brief County Name Getter
     * @return county
     */
    std::string getCounty() const;
    
    /**
     * @brief Check if this record is further north than another
     * @param other [IN] Record to compare against
     * @return true if this record has higher latitude
     */
    bool isNorthOf(const ZipCodeRecord& other) const;
    
    /**
     * @brief Check if this record is further east than another
     * @param other [IN] Record to compare against
     * @return true if this record has smaller longitude (more eastward)
     */
    bool isEastOf(const ZipCodeRecord& other) const;
    
    /**
     * @brief Check if this record is further south than another
     * @param other [IN] Record to compare against
     * @return true if this record has lower latitude
     */
    bool isSouthOf(const ZipCodeRecord& other) const;
    
    /**
     * @brief Check if this record is further west than another
     * @param other [IN] Record to compare against
     * @return true if this record has larger longitude (more westward)
     */
    bool isWestOf(const ZipCodeRecord& other) const;
    
    /**
     * @brief Output stream operator
     * @param outputStream [IN,OUT] Output stream
     * @param record [IN] Record to output
     * @return Reference to output stream
     */
    friend std::ostream& operator<<(std::ostream& outputStream, const ZipCodeRecord& record);
    
    /**
     * @brief Serialize
     * @return Serializes the Zipcode record represented by the ZipCodeRecord and returns that as a uint8_t vector
     */
    std::vector<uint8_t> serialize() const; // Convert to binary format
    /**
     * @brief deserialize
     * @param data the serialized ZipCodeRecord
     * @param length the length of data
     * @return converts data into a ZipCodeRecord and returns that
     */
    static ZipCodeRecord deserialize(const uint8_t* data, size_t length); // Read from binary format

private:
    uint32_t zipCode; // 5-digit zip code
    std::string locationName; // Town name
    std::string county; // County name
    char state[3]; // Two-character state code + null terminator
    double latitude; // Latitude coordinate
    double longitude; // Longitude coordinate  
};

#endif // ZIP_CODE_RECORD_H