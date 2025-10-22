#ifndef BLOCK_BUFFER_H
#define BLOCK_BUFFER_H

#include <vector>
#include <cstdint>
#include <iostream>
#include "ZipCodeRecord.h"

struct ActiveBlock
{
    uint16_t recordCount;
    ActiveBlock* precedingBlock;
    ActiveBlock* succeedingBlock;
    std::vector<ZipCodeRecord> records;
};

struct AvailBlock
{
    uint16_t recordCount;
    AvailBlock* succeedingBlock;
    std::vector<char> padding; // Padding to fill block size
};

class BlockBuffer
{
    public:

    private:

};

#endif // BlockBuffer