#ifndef BLOCK_INDEX_FILE_H
#define BLOCK_INDEX_FILE_H

#include "stdint.h"
#include <iostream>
#include <fstream>
#include <vector>

struct  IndexEntry
{
    uint32_t key; // The key for the index entry
    uint32_t recordRBN; // The RBN of the record in the data file
    uint32_t previousRBN; //previous RBN
    uint32_t nextRBN; //next RBN
};

class BlockIndexFile
{
public:
    /**
     * @brief Default constructor
     */
    BlockIndexFile();

    /**
     * @brief Destructor
     */
    ~BlockIndexFile();

    /**
    * @brief Adds an index entry to the index file
    * @param entry The index entry to add
    */
    void addIndexEntry(const IndexEntry& entry);

    /**
     * @brief Writes the index entries to a file
     * @param filename The name of the file to write to
     * @returns true if write was successful, false otherwise
     */
    bool write(const std::string& filename);

    /**
     * @brief Reads index entries from a file
     * @param filename The name of the file to read from
     * @returns true if read was successful, false otherwise
     */
    bool read(const std::string& filename);

    /**
     * @brief Find RBN for block containing the given zip code
     * @param zipCode zipCode Zip code to search for
     * @return RBN of block that should contain this zip (or -1 if not found)
     */
    uint32_t findRBNForKey(const uint32_t zipCode) const;

private:
    std::vector<IndexEntry> indexEntries; // Vector of index entries


};

#endif // BLOCK_INDEX_FILE_H