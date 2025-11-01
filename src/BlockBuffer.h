#ifndef BLOCK_BUFFER_H
#define BLOCK_BUFFER_H

#include <vector>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "RecordBuffer.h"

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
         * @details Finds the correct block by RBN and reads the record if found
         * @param rbn The RBN of the record to read
         * @return True if the record was found and read successfully
         */
        bool readRecordAtRBN(const uint32_t rbn, const uint32_t zipCode, const uint32_t blockSize, const size_t headerSize, ZipCodeRecord& outRecord);

        /**
         * @brief Writes an active block to the rbn
         * @details Writes the provided block data to the specified RBN in the file
         * @param rbn The RBN of the block to write to
         * @param block The ActiveBlock containing data to write
         * @return True if write was successful
         */
        bool writeActiveBlockAtRBN(const uint32_t rbn, const uint32_t blockSize, const size_t headerSize, const ActiveBlock& block);

        /**
         * @brief Writes an available block to the rbn
         * @details Writes the provided block data to the specifed RBN in the file
         * @param rbn The RBN of the block to write to
         * @param block The AvailBlock containing the data to write
         * @return True if write was succesful
         */
        bool writeAvailBlockAtRBN(const uint32_t rbn, const AvailBlock& block);

        /**
         * @brief Attempts to remove a ZipCodeRecord from a block at a specific RBN
         * @details Finds the correct block by RBN and removes the record if found
         * @param rbn The RBN of the record to remove
         * @return True if the record was found and removed successfully
         */
        bool removeRecordAtRBN(const uint32_t rbn, const uint16_t minBlockSize, uint32_t& availListRBN, const uint32_t zipCode, const uint32_t blockSize, const size_t headerSize);

        /**
         * @brief Attempts to add a ZipCodeRecord to the active blocks
         * @details If there is no space in existing blocks, a new block is created. Finds correct block by key order
         * @param record The ZipCodeRecord to add
         * @return True if the record was added successfully
         */
        bool addRecord(const uint32_t rbn, const uint32_t blockSize, uint32_t& availListRBN, const ZipCodeRecord& record, const size_t headerSize);

        /**
         * @brief Checks if a merge occurred during the last add operation
         * @return True if a merge occurred
         */
        bool getMergeOccurred() const;

        /**
         * @brief Get description of last error
         * @return Error message string reference
         */
        const std::string& getLastError() const;

        /**
         * @brief getter for memory offset
         * @return memory offset
         */
        size_t getMemoryOffset();

        /**
         * @brief Read next block from file
         * @param block [OUT] ActiveBlock to populate
         */
        void readNextActiveBlock(ActiveBlock& block);

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

        /**
         * @brief Dumps the physical order of blocks in the file to standard output
         * @param out [IN] Output stream to write to
         */
        void dumpPhysicalOrder(std::ostream& out) const;

        /**
         * @brief Dumps the logical order of active blocks in the file to standard output
         * @param out [IN] Output stream to write to
         */
        void dumpLogicalOrder(std::ostream& out) const;

        /**
         * @brief Get number of records processed
         * @return Number of records processed
         */
        uint32_t getRecordsProcessed() const;

        /**
         * @brief Get number of blocks processed
         * @return Number of blocks processed
         */
        uint32_t getBlocksProcessed() const;

        /**
         * @brief Loads an active block from the RBN
         * @details Creates a local ActiveBlock to populate with data from the specified RBN in the file
         * @param rbn The RBN of the block to load
         * @return The populated ActiveBlock
         */
        ActiveBlock loadActiveBlockAtRBN(const uint32_t rbn, const uint32_t blockSize, const size_t headerSize);

        /**
         * @brief Loads an available block from the RBN
         * @details Creates a local AvailBlock to populate with data from the specified RBN in the file
         * @param rbn The RBN of the block to load
         * @return The populated AvailBlock
         */
        AvailBlock loadAvailBlockAtRBN(const uint32_t rbn, const uint32_t blockSize, const size_t headerSize);

    private:
        uint32_t recordsProcessed; // Number of records processed from input stream
        uint32_t blocksProcessed; // Number of blocks processed from input stream
        std::fstream blockFile; // Input file stream
        std::string lastError; // Last Error Message
        bool errorState; // Has the Buffer encountered a critical error
        bool mergeOccurred; // Tracks if a merge occurred during last add operation. Likely temporary
        RecordBuffer recordBuffer; // RecordBuffer for packing/unpacking records

        /**
         * @brief Allocates a new block at the end of the file
         * @return RBN of the newly allocated block
         */
        uint32_t allocateBlock(uint32_t& availListRBN); 

        /**
         * @brief Try to merge two blocks
         * @param rbn RBN of the block to be merged
         * @param availListRBN RBN of the avail block for merging
         * @return True if merge was successful
         */
        bool tryJoinBlocks(const uint32_t rbn, uint32_t& availListRBN);

        /**
         * @brief Frees a block at the specified RBN
         * @details Push to available list
         */
        void freeBlock(const uint32_t rbn, uint32_t& availListRBN);

        /**
         * @brief Set error state with message
         * @param message [IN] Error message to set
         */
        void setError(const std::string& message); 

        bool tryBorrowFromPreceding(ActiveBlock& block, ActiveBlock& precedingBlock,
                           std::vector<ZipCodeRecord>& records,
                           std::vector<ZipCodeRecord>& precedingRecords,
                           const uint32_t blockSize, const uint16_t minBlockSize,
                           const size_t headerSize, const uint32_t rbn);

    bool tryBorrowFromSucceeding(ActiveBlock& block, ActiveBlock& succeedingBlock,
                                std::vector<ZipCodeRecord>& records,
                                std::vector<ZipCodeRecord>& succeedingRecords,
                                const uint32_t blockSize, const uint16_t minBlockSize,
                                const size_t headerSize, const uint32_t rbn);
};

#endif // BlockBuffer