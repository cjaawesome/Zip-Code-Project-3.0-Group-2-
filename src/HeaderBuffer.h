#ifndef HEADER_BUFFER_H
#define HEADER_BUFFER_H

#include "HeaderRecord.h"
#include <string>

/**
 * @file HeaderBuffer.h
 * @author Group 2
 * @brief HeaderBuffer class for reading header information
 * @version 0.1
 * @date 2025-10-02
 */


 /**
 * @class HeaderBuffer
 * @brief Buffered reader for processing file headers
 * @details reads, writes, and checks for errors of file headers
 */
class HeaderBuffer 
{
public:
    /**
     * @brief Default constructor
     * @details Initializes header buffer
     */
    HeaderBuffer();

    ~HeaderBuffer();
    
    /**
     * @brief read header
     * @details Opens file, reads header into memory, interprets bytes, uses the header to guide how you read the rest of the file
     * @param filename name of file being read
     * @param header the header being read too
     * @returns true or false depending on if the header was read successfully or not
     */
    bool readHeader(const std::string& filename, HeaderRecord& header);
    /**
     * @brief read header
     * @details Opens file, writes the header, and closes file
     * @param filename name of file being written too
     * @param header the header being written
     * @returns true or false depending on if the header was write was successfully or not
     */
    bool writeHeader(const std::string& filename, const HeaderRecord& header);
    
    /**
     * @brief has error
     * @details returns buffer errorState
     * @returns true or false depending on errorState
     */
    bool hasError() const;

    /**
     * @brief error message
     * @details returns buffer lastError
     * @returns returns the last error message
     */
    std::string getLastError() const;
    
private:
    bool errorState;
    std::string lastError;
    
    /**
     * @brief set Error
     * @details sets error state true a
     */
    void setError(const std::string& message);
};

#endif