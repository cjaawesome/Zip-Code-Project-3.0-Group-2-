#include "PrimaryKeyIndex.h"
#include "ZipCodeRecord.h"
#include <iostream>

#include <fstream>
#include <string>
#include <cstring>
#include <vector>

/**
 * @file PrimaryKeyIndex.cpp
 * @author Group 2
 * @brief PrimaryKeyIndex for storing 
 * @version 0.1
 * @date 2025-10-12
 */

void PrimaryKeyIndex::createFromDataFile(CSVBuffer& buffer)
{
    ZipCodeRecord record;
    size_t dataOffset = buffer.getMemoryOffset();

    int count = 0;
    while (buffer.getNextLengthIndicatedRecord(record)) {
        uint32_t zip = record.getZipCode();
        
        listAddManager(record, dataOffset);
        
        dataOffset = buffer.getMemoryOffset(); // Update for next record
        count++;
    }
}

bool PrimaryKeyIndex::write(const std::string& filename)
{
    std::ofstream out(filename, std::ios::binary);
    if (!out) return false;

    // Write count of secondary entries
    size_t secCount = secondaryEntries.size();
    out.write(reinterpret_cast<const char*>(&secCount), sizeof(secCount));
    
    // Write secondary entries (field by field)
    for (const auto& s : secondaryEntries) 
    {
        out.write(reinterpret_cast<const char*>(&s.zip), sizeof(s.zip));
        out.write(reinterpret_cast<const char*>(&s.arrayIndex), sizeof(s.arrayIndex));
    }

    // Write count of primary entries
    size_t primCount = primaryEntries.size();
    out.write(reinterpret_cast<const char*>(&primCount), sizeof(primCount));
    
    // Write primary entries (field by field)
    for (const auto& p : primaryEntries) 
    {
        out.write(reinterpret_cast<const char*>(&p.offset), sizeof(p.offset));
        out.write(reinterpret_cast<const char*>(&p.nextIndex), sizeof(p.nextIndex));
    }

    out.close();
    return true;
}

bool PrimaryKeyIndex::read(const std::string& filename)
{
    std::ifstream in(filename, std::ios::binary);
    if (!in) return false;

    secondaryEntries.clear();
    primaryEntries.clear();

    // Read secondary count and entries
    size_t secCount;
    in.read(reinterpret_cast<char*>(&secCount), sizeof(secCount));
    if (!in) return false;
    
    for (size_t i = 0; i < secCount; ++i) 
    {
        SecondaryIndexEntry s;
        in.read(reinterpret_cast<char*>(&s.zip), sizeof(s.zip));
        in.read(reinterpret_cast<char*>(&s.arrayIndex), sizeof(s.arrayIndex));
        if (!in) return false;
        secondaryEntries.push_back(s);
    }

    // Read primary count and entries
    size_t primCount;
    in.read(reinterpret_cast<char*>(&primCount), sizeof(primCount));
    if (!in) return false;
    
    for (size_t i = 0; i < primCount; ++i) 
    {
        PrimaryIndexEntry p;
        in.read(reinterpret_cast<char*>(&p.offset), sizeof(p.offset));
        in.read(reinterpret_cast<char*>(&p.nextIndex), sizeof(p.nextIndex));
        if (!in) return false;
        primaryEntries.push_back(p);
    }

    return true;
}

std::vector<size_t> PrimaryKeyIndex::find(uint32_t zip) const
{
    std::vector<size_t> addresses;
    int index = secondaryContains(zip);
    
    if (index != -1){
        int primIndex = secondaryEntries[index].arrayIndex;
        
        while(primaryEntries[primIndex].nextIndex != -1)
        {
            addresses.push_back(primaryEntries[primIndex].offset);
            primIndex = primaryEntries[primIndex].nextIndex;
        }
        addresses.push_back(primaryEntries[primIndex].offset);
    }
    return addresses;
}

bool PrimaryKeyIndex::contains(const uint32_t zip) const{
    return (secondaryContains(zip) != -1);
}

void PrimaryKeyIndex::listAddManager(const ZipCodeRecord& zipRecord, const size_t& memoryOffset)
{
    SecondaryIndexEntry sEntry;
    PrimaryIndexEntry pEntry;
    
    int sIndex = secondaryContains(zipRecord.getZipCode());
    
    if(sIndex == -1)
    {
        sEntry.zip = zipRecord.getZipCode();
        sEntry.arrayIndex = -1;
        sIndex = addSecondarySorted(sEntry); // ← YOU'RE NOT CAPTURING THE RETURN!
    }
    
    pEntry.offset = memoryOffset;
    pEntry.nextIndex = -1;
    addPrimary(secondaryEntries[sIndex], pEntry);
}

int PrimaryKeyIndex::secondaryContains(const uint32_t zip) const{
    if (secondaryEntries.size() == 0) return -1; //if list is empty not found
    int left = 0;
    int right = secondaryEntries.size() - 1;

    //basic binary search
    while (left <= right) {
        int mid = left + (right - left) / 2; // avoid overflow

        if (secondaryEntries[mid].zip == zip) {
            return mid; // found
        }
        else if (secondaryEntries[mid].zip < zip) {
            left = mid + 1; // search right half
        }
        else {
            right = mid - 1; // search left half
        }
    }
    return -1; // not found
}

int PrimaryKeyIndex::addSecondarySorted(const SecondaryIndexEntry& entry){
    int size = secondaryEntries.size();
    if (size == 0){ //if empty add to list
        secondaryEntries.push_back(entry);
        return size;
    }
    //if not empty add into list sorted
    int index = 0;
    while(index < secondaryEntries.size()){
        
        if(secondaryEntries[index].zip > entry.zip){
            secondaryEntries.insert(secondaryEntries.begin() + index, entry); //
            return index;
        }
        index++;
    }
    secondaryEntries.push_back(entry);
    return size;
}

int PrimaryKeyIndex::addPrimary(SecondaryIndexEntry& sEntry, const PrimaryIndexEntry& pEntry){
    int size = primaryEntries.size();
    if (size == 0 || sEntry.arrayIndex == -1){ //if list is empty or secondary index chain hasn't started push back and update sEntry
        primaryEntries.push_back(pEntry);
        sEntry.arrayIndex = size;
        return size;
    }
    //if not empty and chain has started traverse to end of chain of primary indexs and adds to the end of list
    int index = sEntry.arrayIndex;
    while(primaryEntries[index].nextIndex != -1){
        index = primaryEntries[index].nextIndex;
    }
    primaryEntries.push_back(pEntry);
    primaryEntries[index].nextIndex = size;
    return size;
}