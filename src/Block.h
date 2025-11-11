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
        return 10 + data.size();  // Metadata + data
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