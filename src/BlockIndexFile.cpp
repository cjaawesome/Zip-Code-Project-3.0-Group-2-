#include "BlockIndexFile.h"
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>

const std::string ENDOFFILE = "|";

BlockIndexFile::BlockIndexFile(){    
}

BlockIndexFile::~BlockIndexFile(){    
}

bool BlockIndexFile::createIndexFromBlockedFile(const std::string& zcbFilePath,
                                               uint32_t blockSize,
                                               size_t headerSize,
                                               uint32_t sequenceSetHead)
{
    indexEntries.clear();  // Clear any existing entries
    
    BlockBuffer blockBuffer;
    RecordBuffer recordBuffer;
    
    if (!blockBuffer.openFile(zcbFilePath, headerSize)) {
        return false;
    }
    
    uint32_t currentRBN = sequenceSetHead;
    while(currentRBN != 0)
    {
        ActiveBlock block = blockBuffer.loadActiveBlockAtRBN(currentRBN, blockSize, headerSize);
        
        std::vector<ZipCodeRecord> records;
        recordBuffer.unpackBlock(block.data, records);
        
        if (!records.empty()) 
        {
            IndexEntry entry;
            entry.recordRBN = currentRBN;
            entry.key = records.back().getZipCode();  // Highest zip in block
            indexEntries.push_back(entry);
        }
        
        currentRBN = block.succeedingRBN;
    }
    
    // Sort by key (should already be sorted if blocks are, but safe)
    std::sort(indexEntries.begin(), indexEntries.end(),
        [](const IndexEntry& a, const IndexEntry& b) 
        {
            return a.key < b.key;
        });
    
    blockBuffer.closeFile();
    return true;
}

void BlockIndexFile::addIndexEntry(const IndexEntry& entry){
    if(indexEntries.empty()){
        indexEntries.push_back(entry);
        return;
    }

    for(int i = 0; i < indexEntries.size(); i++){
        if(indexEntries[i].key > entry.key){
            indexEntries.insert(indexEntries.begin() + i, entry);
            return;
        }
    }
}


bool BlockIndexFile::write(const std::string& filename){
    std::ofstream file;
    file.open(filename, std::ios::out);
    if(!file){
        return false;
    }

    for(const auto& index : indexEntries){ //format { key recordRBN }
        file << "{ " << index.key << " ";
        file << index.recordRBN << " ";
        file << index.previousRBN << " ";
        file << index.nextRBN << " } ";
    }   
    file << ENDOFFILE;
    file.close();

    return true;
}

bool BlockIndexFile::read(const std::string& filename){
    std::ifstream file;
    file.open(filename, std::ios::in);
    indexEntries.clear(); //clear list
    if(!file){
        return false;
    }
    std::string current;
    file >> current; //read in first "{"
    while(current != ENDOFFILE){
        IndexEntry index;
        // Read key, recordRBN, previousRBN, nextRBN
        file >> index.key;
        file >> index.recordRBN;
        file >> index.previousRBN;
        file >> index.nextRBN;

        indexEntries.push_back(index); //add new index to list

        file >> current; //read in "}"
        file >> current; //read in next "{" or EOF marker
    }
    file.close();

    return true;
}


uint32_t BlockIndexFile::findRBNForKey(const uint32_t zipCode) const
{
    for(const auto& index : indexEntries)
    {
        if(zipCode <= index.key) 
            return index.recordRBN;
    }
    return -1;
}