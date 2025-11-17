#ifndef RECORD_BUFFER_H
#define RECORD_BUFFER_H

#include "stdint.h"
#include "Block.h"
#include "ZipCodeRecord.h"
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>

class RecordBuffer
{
public:
    static const int EXPECTED_FIELD_COUNT = 6;
    static const char* const EXPECTED_HEADERS[EXPECTED_FIELD_COUNT];
    /**
     * @brief Default constructor
     */
    RecordBuffer();

    /**
     * @brief Destructor
     */
    ~RecordBuffer();

    /**
     * @brief Unpack block data into ZipCodeRecords
     * @param blockData [IN] Raw block data
     * @param records [OUT] Vector to populate with unpacked records
     * @return True if unpacking was successful
     */
    bool unpackBlock(const std::vector<char>& blockData, std::vector<ZipCodeRecord>& records);

    /**
     * @brief Pack ZipCodeRecords into block data
     * @param records [IN] Vector of ZipCodeRecords to pack
     * @param blockData [OUT] Vector to populate with packed block data
     * @param blockSize The constant size of the blocks in the file in bytes.
     * @return True if packing was successful
     */
    bool packBlock(const std::vector<ZipCodeRecord>& records, std::vector<char>& blockData, const uint32_t blockSize);
    
    /**
     * @brief Checks if the buffer is in an error state
     * @return True if an error has occurred
     */
    bool hasError() const;

    /**
     * @brief Get description of last error
     * @return Error message string reference
     */
    std::string getLastError() const;

    /**
     * @brief Parses comma seperated ZipCodeRecord.
     * @param recordStr The incoming ZipCode data as a string
     * @param record The incoming record object to be modified
     * @return True if parsing was successful
     */
    bool parseZipCodeRecord(const std::string& recordStr, ZipCodeRecord& record);

private:
    bool errorState; // Has the RecordBuffer encountered a critical error
    std::string lastError; // Last error message thrown by the error record

    /**
     * @brief Convert string fields
     * @param fields [IN] Vector of string fields from data file
     * @param record [OUT] ZipCodeRecord to populate
     * @return true if conversion successful
     * @pre fields must contain exactly 6 valid elements
     * @post record is populated with converted data
     */
     bool fieldsToRecord(const std::vector<std::string>& fields, ZipCodeRecord& record);
        
     /**
     * @brief Validate that a string represents a valid integer
     * @param str [IN] String to validate
     * @return true if string is a valid integer
     */
     bool isValidUInt32(const std::string& str) const;
        
    /**
     * @brief Validate that a string represents a valid double
     * @param str [IN] String to validate  
     * @return true if string is a valid double
     */
     bool isValidDouble(const std::string& str) const;

    /**
     * @brief Set error state and message
     * @param message [IN] Error message to set
     */
     void setError(const std::string& message);

    /**
     * @brief Trim whitespace from string
     * @param str [IN,OUT] String to trim
     * @details Removes leading and trailing whitespace
     */
    void trimString(std::string& str);
};

#endif // RECORD_BUFFER_H