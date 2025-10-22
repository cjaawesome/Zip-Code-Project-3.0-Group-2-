#ifndef CSV_BUFFER_H
#define CSV_BUFFER_H

#include "ZipCodeRecord.h"
#include "stdint.h"
#include <vector>
#include <fstream>
#include <string>
#include <sstream>

/**
 * @file CSVBuffer.h
 * @author Group 2
 * @brief CSVBuffer class for reading zip code records from CSV files
 * @version 0.1
 * @date 2025-09-11
 */

/**
 * @class CSVBuffer
 * @brief Buffered reader for processing zip code CSV files sequentially
 * @details Reads CSV files line by line, parses comma-separated values,
 *          and converts them into ZipCodeRecord objects. Handles header
 *          row skipping and provides efficient sequential access.
 */
class CSVBuffer
{
public:
    // Header field constants for validation (ie stored class members)
    static const int EXPECTED_FIELD_COUNT = 6;
    static const char* const EXPECTED_HEADERS[EXPECTED_FIELD_COUNT];
    
    /**
     * @brief Default constructor
     * @details Initializes buffer in closed state
     * @post Buffer is ready to open a file
     */
    CSVBuffer();
    
    /**
     * @brief Constructor with filename
     * @param filename [IN] Path to CSV file to open
     * @details Automatically opens the specified file and skips header
     * @post File is open and positioned after header, or error state if failed
     */
    explicit CSVBuffer(const std::string& filename, uint32_t headerSize);
    
    /**
     * @brief Destructor
     * @details Automatically closes file if open
     */
    ~CSVBuffer();
    
    /**
     * @brief Open CSV file for reading
     * @param filename [IN] Path to CSV file
     * @return true if file opened successfully and header is valid
     * @pre File must exist and be readable
     * @post File is open and positioned after header row, or closed if error
     * @note Automatically skips and validates header row
     */
    bool openFile(const std::string& filename);
    
    /**
     * @brief Read next zip code record from file
     * @param record [OUT] ZipCodeRecord object to populate
     * @return true if record was successfully read, false if EOF or error
     * @pre File must be open and positioned at data row
     * @post File position advanced to next line, record populated if successful
     * @details Parses comma-separated values and validates data before creating record
     */
    bool getNextRecord(ZipCodeRecord& record);
    
    /**
     * @brief Check if more records are available
     * @return true if more data can be read, false if EOF or error
     * @pre File must be open
     * @post File position unchanged
     */
    bool hasMoreRecords() const;
    
    /**
     * @brief Close the CSV file
     * @pre None
     * @post File is closed, buffer is reset
     */
    void closeFile();
    
    /**
     * @brief Get current line number being processed
     * @return Current line number (1-based, excluding header)
     * @details Useful for error reporting and progress tracking
     */
    uint32_t getCurrentLineNumber() const;
    
    /**
     * @brief Get total number of records successfully read
     * @return Count of records processed
     */
    uint32_t getRecordsProcessed() const;
    
    /**
     * @brief Check if buffer is in error state
     * @return true if last operation failed
     */
    bool hasError() const;
    
    /**
     * @brief Get description of last error
     * @return Error message string
     */
    std::string getLastError() const;

    /**
     * @brief Open length-indicated file for reading
     * @param filename [IN] Path to .zcd file
     * @param header [OUT] HeaderRecord to populate
     * @return true if file opened and header read successfully
     */
    bool openLengthIndicatedFile(const std::string& filename, uint32_t headerSize);
    
    /**
     * @brief Read next record from length-indicated file
     * @param record [OUT] ZipCodeRecord object to populate
     * @return true if record was successfully read
     */
    bool getNextLengthIndicatedRecord(ZipCodeRecord& record);

    /**
     * @brief getter for memory offset
     * @return memory offset
     */
    size_t getMemoryOffset();

    /**
     * @brief reads a record at a specific memory address
     * @param address the memory address of the record being read
     * @param record the ZipCodeRecord being populated
     * @return true if successfuly reads the record
     */
    bool readRecordAtMemoryAddress(const size_t address, ZipCodeRecord& record);

private:
    std::ifstream csvFile; // Input file stream
    std::string currentLine; // Current line buffer
    uint32_t lineNumber; // Current line number (1-based)
    uint32_t recordsProcessed; // Count of successfully processed records
    bool errorState; // Error flag
    bool isLengthIndicatedMode;  // Track read mode
    std::string lastError; // Last error message
    /**
     * @brief Skip and validate CSV header row
     * @return true if header is valid and skipped successfully
     * @pre File must be open and positioned at beginning
     * @post File positioned after header row
     * @details Reads header lines and validates expected field structure
     */
    bool skipHeader();
    
    /**
     * @brief Parse comma-separated line into individual fields
     * @param line [IN] CSV line to parse
     * @param fields [OUT] Vector to store parsed fields
     * @return true if parsing successful
     * @details Handles quoted fields and comma separation
     */
    bool parseLine(const std::string& line, std::vector<std::string>& fields);
    
    /**
     * @brief Convert string fields to ZipCodeRecord
     * @param fields [IN] Vector of string fields from CSV
     * @param record [OUT] ZipCodeRecord to populate
     * @return true if conversion successful
     * @pre fields must contain exactly 6 valid elements
     * @post record is populated with converted data
     */
    bool fieldsToRecord(const std::vector<std::string>& fields, ZipCodeRecord& record);
    
    /**
     * @brief Trim whitespace from string
     * @param str [IN,OUT] String to trim
     * @details Removes leading and trailing whitespace
     */
    void trimString(std::string& str);
    
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
     * @brief Set error state with message
     * @param message [IN] Error description
     * @post errorState is true, lastError contains message
     */
    void setError(const std::string& message);
};

#endif // CSV_BUFFER_