#ifndef PRIMARY_KEY_INDEX
#define PRIMARY_KEY_INDEX

#include "CSVBuffer.h"
#include <iostream>
#include <map>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include "stdint.h"

/**
 * @file PrimaryKeyIndex.h
 * @author Group 2
 * @brief PrimaryKeyIndex for storing 
 * @version 0.1
 * @date 2025-10-12
 */

/**
 * @class PrimaryKeyIndex
 * @brief Represents the primary key index of a file
 * @details maps zipcodes and there memory offsets
 */
class PrimaryKeyIndex {
public:
    //struct representation of a secondary index entry
    struct SecondaryIndexEntry {
        int zip; //secondary index
        int arrayIndex; // index in array 
    };
    //struct representation of a primary index entry
    struct PrimaryIndexEntry {
        size_t offset; //memory offset
        int nextIndex; // next index in array
    };
    /**
     * @brief reads data from a cvsbuffer and creates a primary index for it
     * @param buffer the CSV file reading in the zipcode records
     */
    void createFromDataFile(CSVBuffer& buffer);
    /**
     * @brief saves the index to a binary index file
     * @return true if successful write to file
     */
    bool write(const std::string& filename);
    /**
     * @brief reads index from a binary index file
     * @return true if successfuly reads from file
     */
    bool read(const std::string& filename);
    /**
     * @brief finds zip code in entries and returns memory offsets with the matching zip
     * @param zip zip code being searched for
     * @return returns a vector of all of the primaryindexentries of that zip code
     */
    std::vector<size_t> find(const uint32_t zip) const;
    /**
     * @brief searches if a zip code is in the map
     * @param zip zip code being searched for
     * @return returns true if in the map
     */
    bool contains(const uint32_t zip) const;
    
private:
    std::vector<SecondaryIndexEntry> secondaryEntries; //secondary keys
    std::vector<PrimaryIndexEntry> primaryEntries; //primary keys
    

    /**
     * @brief manager for adding to primary and secondary entries
     * @param zipRecord zip code record being added to Index Entry
     * @param memoryOffset offset of the zipcode record in file
     */
    void listAddManager(const ZipCodeRecord& zipRecord, const size_t& memoryOffset);
    /**
     * @brief returns index of zip code in secondary index if it contains it
     * @param zip zip being searched for in secondary
     * @return index of zipcode in secondary index vector (-1 if not in vector)
     */
    int secondaryContains(const uint32_t zip) const;  
    /**
     * @brief handles adding to secondary index vector in a sorted way
     * @param entry entry being added
     * @return index of entry in secondary vector
     */
    int addSecondarySorted(const SecondaryIndexEntry& entry);
    /**
     * @brief handles adding to the primary index
     * @param sEntry secondary index being extended by primary index
     * @param pEntry primary index being added to primary vector
     * @return index of entry in secondary vector
     */
    int addPrimary(SecondaryIndexEntry& sEntry, const PrimaryIndexEntry& pEntry);
};

#endif


