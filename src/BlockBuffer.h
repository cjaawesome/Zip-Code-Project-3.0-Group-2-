#ifndef BLOCK_BUFFER_H
#define BLOCK_BUFFER_H

#include <vector>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "ZipCodeRecord.h"

struct ActiveBlock
{
    uint16_t recordCount; // Records held by this block (techncially redundant could be fetched from records vector)
    ActiveBlock* precedingBlock; // Pointer to prior active block
    ActiveBlock* succeedingBlock; // Pointer to succeeding active block
    std::vector<ZipCodeRecord> records; // Records Ordered by Key
};

struct AvailBlock
{
    uint16_t recordCount; // Records held by this block
    AvailBlock* succeedingBlock; // Pointer to succeeding active block
    std::vector<char> padding; // Padding to fill block size
};

class BlockBuffer
{
    public:
        /**
         * @brief Default constructor
         */
        BlockBuffer();
        /**
         * @brief Destructor
         */
        ~BlockBuffer();

        /**
         * @brief Open file for reading
         * @param filename [IN] Path to block file
         * @return True if file opened successfully
         */
        bool openFile(const std::string& filename, const size_t headerSize);

        /**
         * @brief Check if there is more data in the file
         * @return True if more data is available
         */
        bool hasMoreData() const;
        
        /**
         * @brief Checks if the buffer is in an error state
         * @return True if an error has occurred
         */
        bool hasError() const;

        /**
         * @brief Attempts to read a ZipCodeRecord from a block at a specific RBN
         * @details Finds the correct block by RBN and reads the record if found.
         * @param rbn The RBN of the record to read
         * @return True if the record was found and read successfully
         */
        bool readRecordAtRBN(const uint32_t rbn, ZipCodeRecord& record);

        /**
         * @brief Attempts to remove a ZipCodeRecord from a block at a specific RBN
         * @details Finds the correct block by RBN and removes the record if found
         * @param rbn The RBN of the record to remove
         * @return True if the record was found and removed successfully
         */
        bool removeRecordAtRBN(const uint32_t rbn);

        /**
         * @brief Attempts to add a ZipCodeRecord to the active blocks
         * @details If there is no space in existing blocks, a new block is created. Finds correct block by key order
         * @param record The ZipCodeRecord to add
         * @return True if the record was added successfully
         */
        bool addRecord(const ZipCodeRecord& record);

        /**
         * @brief Checks if a merge occurred during the last add operation
         * @return True if a merge occurred
         */
        bool getMergeOccurred() const;

        /**
         * @brief Get description of last error
         * @return Error message string
         */
        std::string getLastError() const;

        /**
         * @brief getter for memory offset
         * @return memory offset
         */
        size_t getMemoryOffset();

        /**
         * @brief Read next block from file
         * @param block [OUT] ActiveBlock to populate
         */
        void readNextBlock(ActiveBlock& block);

        /**
         * @brief Read next available block from file
         * @details Populates a Block structure with data from the file
         * @param block [OUT] AvailBlock to populate
         */
        void readNextAvailBlock(AvailBlock& block);

        /**
         * @brief Close the currently opened file
         */
        void closeFile();
    private:
        uint32_t recordsProcessed; // Number of records processed from input stream
        uint32_t blocksProcessed; // Number of blocks processed from input stream
        std::ifstream blockFile; // Input file stream
        std::string lastError; // Last Error Message
        bool errorState; // Has the Buffer encountered a critical error
        bool mergeOccurred; // Tracks if a merge occurred during last add operation. Likely temporary.
        ActiveBlock* activeBlockHead; // Head of active block linked list

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
         * @brief Helper function that attempt to join two blocks
         * @details Merges records from block2 into block1 if space allows and updates pointers.
         * @param block1 [IN,OUT] First block to join
         * @param block2 [IN,OUT] Second block to join
         * @return True if blocks were joined successfully
         */
        bool tryJoinBlocks(ActiveBlock& block1, ActiveBlock& block2);

        /**
         * @brief Set error state with message
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

#endif // BlockBuffer