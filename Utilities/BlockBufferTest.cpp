#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

#include "../src/BlockBuffer.h"
#include "../src/Block.h"

int main()
{
    const std::string filename = "test_blocks.zcd";
    const size_t headerSize = 128;
    const uint32_t blockSize = 512;
    const uint32_t blockCount = 2; // we'll create RBN 1..2

    // Create a small test block file with a header and two active blocks
    {
        std::ofstream out(filename, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            std::cerr << "Failed to create test file\n";
            return 1;
        }

        // Write header (just zeros)
        std::vector<char> header(headerSize, 0);
        out.write(header.data(), static_cast<std::streamsize>(header.size()));

        // Write an empty block for RBN 0 (placeholder)
        std::vector<char> emptyBlock(blockSize, 0);
        out.write(emptyBlock.data(), static_cast<std::streamsize>(emptyBlock.size()));

        // Helper to write an active block for a given rbn
        auto writeActive = [&](uint16_t recCount, uint32_t preceding, uint32_t succeeding, const std::string& payload){
            std::vector<char> block(blockSize, 0);
            size_t off = 0;
            // recordCount
            memcpy(block.data() + off, &recCount, sizeof(recCount)); off += sizeof(recCount);
            // precedingRBN
            memcpy(block.data() + off, &preceding, sizeof(preceding)); off += sizeof(preceding);
            // succeedingRBN
            memcpy(block.data() + off, &succeeding, sizeof(succeeding)); off += sizeof(succeeding);
            // payload
            size_t pLen = std::min(payload.size(), blockSize - off);
            memcpy(block.data() + off, payload.data(), pLen);

            out.write(block.data(), static_cast<std::streamsize>(block.size()));
        };

        // Write RBN 1
        writeActive(1, 0, 2, "PAYLOAD_BLOCK_1");
        // Write RBN 2
        writeActive(1, 1, 0, "PAYLOAD_BLOCK_2");

        out.close();
        std::cout << "Wrote test file: " << filename << "\n";
    }

    // Now open the file with BlockBuffer and read blocks
    BlockBuffer buffer;
    if (!buffer.openFile(filename, headerSize)) {
        std::cerr << "BlockBuffer failed to open file: " << buffer.getLastError() << "\n";
        return 1;
    }

    // Read and dump block 1 and 2
    for (uint32_t rbn = 1; rbn <= blockCount; ++rbn)
    {
        ActiveBlock block = buffer.loadActiveBlockAtRBN(rbn, blockSize, headerSize);
        std::cout << "RBN " << rbn << ": recordCount=" << block.recordCount
                  << " preceding=" << block.precedingRBN
                  << " succeeding=" << block.succeedingRBN << "\n";
        if (!block.data.empty()) {
            // print first bytes of payload as string up to a null or 64 chars
            size_t len = block.data.size();
            size_t maxPrint = std::min<size_t>(len, 64);
            std::string payload(block.data.begin(), block.data.begin() + maxPrint);
            // Trim trailing zeros for nicer output
            size_t firstZero = payload.find('\0');
            if (firstZero != std::string::npos) payload.resize(firstZero);
            std::cout << "  payload: '" << payload << "' (" << len << " bytes stored)\n";
        } else {
            std::cout << "  payload: <empty>\n";
        }
    }

    // Try dumpPhysicalOrder if available
    try {
        buffer.dumpPhysicalOrder(std::cout, 1, 0, blockCount, blockSize, headerSize);
    } catch (...) {
        std::cerr << "dumpPhysicalOrder threw an exception or is not implemented\n";
    }

    buffer.closeFile();
    std::cout << "Test finished.\n";
    return 0;
}
