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

void BlockIndexFile::addIndexEntry(const IndexEntry& entry){
    if(indexEntries.empty()){
        indexEntries.push_back(entry);
        return;
    }

    for(int i = 0; i < indexEntries.size(); i++){
        if(indexEntries[i].key < entry.key){
            indexEntries.insert(indexEntries.begin() + i, entry);
            return;
        }
    }
}


bool BlockIndexFile::write(const std::string& filename){
    std::ofstream file(filename);
    if(!file){
        return false;
    }

    for(const auto& index : indexEntries){ //format { key recordRBN }
        file << "{ " << index.key << " ";
        file << index.recordRBN << " }" << " ";
    }
    file << ENDOFFILE;

    return true;
}

bool BlockIndexFile::read(const std::string& filename){
    std::ifstream file(filename);
    indexEntries.clear(); //clear list
    if(!file){
        return false;
    }
    std::string current;
    file >> current; //read in first "{"
    while(current != ENDOFFILE){
        IndexEntry index;
        file >> current; //read in key
        index.key = std::stoi(current);
        file >> current; //read in recordRBN
        index.recordRBN = std::stoi(current);

        indexEntries.push_back(index); //add new index to list

        file >> current; //read in "}"
        file >> current; //read in next "{" or EOF marker
    }

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