#ifndef BLOCK_H
#define BLOCK_H

#include "stdint.h"
#include <vector>

struct ActiveBlock
{
    uint16_t recordCount; // Records held by this block (techncially redundant could be fetched from records vector)
    uint32_t precedingRBN; // Pointer to prior active block
    uint32_t succeedingRBN; // Pointer to succeeding active block
    std::vector<char> data; // Raw Block Data
    size_t getTotalSize() const 
    {
    // Find first padding byte (0xFF)
    size_t actualDataSize = data.size();
        for(size_t i = data.size(); i > 0; --i) 
        {
            if(data[i-1] != '\xFF') 
            {
                actualDataSize = i;
                break;
            }
        }
        return 10 + actualDataSize;  // Metadata + actual data (no padding)
    }
};

struct AvailBlock
{
    uint16_t recordCount; // Records held by this block
    uint32_t succeedingRBN; // Pointer to succeeding active block
    std::vector<char> padding; // Padding to fill block size
    size_t getTotalSize() const 
    {
        return 6 + padding.size();  // Metadata + data
    }
};

#endif