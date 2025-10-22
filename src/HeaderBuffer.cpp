#include "HeaderBuffer.h"
#include <fstream>
#include <vector>

HeaderBuffer::HeaderBuffer() : errorState(false), lastError("") {}

HeaderBuffer::~HeaderBuffer()
{
    // Destructor
}

bool HeaderBuffer::readHeader(const std::string& filename, HeaderRecord& header)
{
     std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        setError("Cannot open file: " + filename);
        return false;
    }

    // Read enough bytes to get to headerSize field
    constexpr size_t MIN_HEADER_READ = 10;
    std::vector<uint8_t> buffer(MIN_HEADER_READ);

    file.read(reinterpret_cast<char*>(buffer.data()), MIN_HEADER_READ);
    if (file.gcount() < MIN_HEADER_READ) 
    {
        setError("File too small to contain a valid header");
        return false;
    }

    // Extract headerSize
    uint32_t headerSize = 0;
    std::memcpy(&headerSize, buffer.data() + 6, sizeof(headerSize));

    // Read full header based on headerSize 
    buffer.resize(headerSize);
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(buffer.data()), headerSize);
    if (file.gcount() < static_cast<std::streamsize>(headerSize)) 
    {
        setError("Header incomplete");
        return false;
    }

    file.close();

    // Deserialize header
    header = HeaderRecord::deserialize(buffer.data());
    return true;
}

bool HeaderBuffer::writeHeader(const std::string& filename, const HeaderRecord& header)
{
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) 
    {
        setError("Cannot create file: " + filename);
        return false;
    }
    
    auto headerData = header.serialize();
    file.write(reinterpret_cast<char*>(headerData.data()), headerData.size());
    file.close();
    
    return true;
}

bool HeaderBuffer::hasError() const 
{
    return errorState;
}

std::string HeaderBuffer::getLastError() const 
{
    return lastError;
}

void HeaderBuffer::setError(const std::string& message) 
{
    errorState = true;
    lastError = message;
}