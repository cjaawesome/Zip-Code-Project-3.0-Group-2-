#include "HeaderRecord.h"

HeaderRecord::HeaderRecord()
{
}

HeaderRecord::~HeaderRecord()
{
}

std::vector<uint8_t> HeaderRecord::serialize() const
{
/*
    Explanation of patterns:
        - data.insert(data.end(), reinterpret_cast<const uint8_t*>(&x), reinterpret_cast<const uint8_t*>(&x) + sizeof(x)):
            - The line of code above does the following:
                - data.end() is the starting location of the insertion. So this means insertion is at the end of vector.
                - The first reinterpret cast is the starting address of the variable.
                - The secoind reinterpet cast is the end adress of the variable.
                - The insert then copies all bytes between the starting location and ending location into the vector.
                - It needs to be reinterpreted to a uint8_t to maintain the binary format.
                - Example: 
                    - Version is stored as a uint16_t. The structure above will split the uint16_t into two uint_8t's.
                    - Those two uint8_t's will then be added as the next two entries in the vector.

        - data.insert(data.end(), fileStructureType, fileStructureType + 4);
            - File Structure type is an array of characters:
                - Since each character is one byte it does not need a reinterpret cast.
                - So the start is the beginning of fileStructureType (arrays are adresses) .
                - The end is fileStructureType + 4 (which is the array length).
                - Insert will add each byte inbetween these to the vector.

        - data.push_back(sizeFormatType);
            - sizeFormatType is already a uint8_t meaning it can be pushed without modifcation.
        
        - data.insert(data.end(), indexFileName.begin(), indexFileName.end());
            - Again the start position is the end of the vector.
            - using the .begin() function on a string gives the address of the first stored char.
            - using the .end() function on a string gives the address of the last stored char.
            - In conjuction they accomplish the same thing as fileStructureType, only it is dynamic.

        -  size_t headerSizePos = data.size();
            uint32_t tempHeaderSize = 0;
            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&tempHeaderSize),
                        reinterpret_cast<const uint8_t*>(&tempHeaderSize) + sizeof(tempHeaderSize)

            - This stores the starting memory address of headerSizePos by getting the data vectors current size.
            - Then it creates a temporary headerSizeVariable as a uint32_t to insert for now.
            - It is inserted using the usual pattern.
            - Once the rest of the data has been inserted the memory address will be used to change it to the calculated value.

        - for(const auto& field : fields)
            {
                // Length of name field
                uint16_t nameLength = field.name.length();
                data.insert(data.end(), reinterpret_cast<const uint8_t*>(&nameLength),
                         reinterpret_cast<const uint8_t*>(&nameLength) + sizeof(nameLength));

                // Name of field
                data.insert(data.end(), field.name.begin(), field.name.end());

                // Field Type
                data.push_back(field.type);
            }
            
            - So for each field in the fields array:
                - The name length is fetched from the filed name in the structure and inserted using the reinterpret pattern.
                - The name of the field is fetched from the structure and inserted using the string pattern.
                - The field type is fetched from the structure and inserted using push_back as it is already a uint8_t.

        -  uint32_t trueHeaderSize = data.size();
            memcpy(&data[headerSizePos], &trueHeaderSize, sizeof(trueHeaderSize));

            - Here a new uint32_t that will represent the true size of the header is created and set to the current dataSize.
            - Breaking down memcpy:
                - &data[headerSizePos]:
                    - Location in the data vector where the temp variable was stored
                - &trueHeaderSize:
                    - Reference to the calculated size that will be replacing the temp variable
                - sizeof(trueHeaderSize):
                    - The amount of bytes that will be copied. In this case 4 bytes for the uint32_t.

    That should just about cover everything going on here, couple more things:
        - There is no error checking or validation going on at the moment.
        - Everything here is converted to little endian format. It is not fully portable.
*/

    std::vector<uint8_t> data; // Stores the binary data

    // File Structure Type
    data.insert(data.end(), fileStructureType, fileStructureType + 4);

    // Version
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&version), 
                reinterpret_cast<const uint8_t*>(&version) + sizeof(version));

    // Create Header Size and Store location since it needs to be calculated
    size_t headerSizePos = data.size();
    uint32_t tempHeaderSize = 0;
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&tempHeaderSize),
                reinterpret_cast<const uint8_t*>(&tempHeaderSize) + sizeof(tempHeaderSize));

    // Size format type
    data.push_back(sizeFormatType);

    // Block Size
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&blockSize),
                reinterpret_cast<const uint8_t*>(&blockSize) + sizeof(blockSize));

    // Min Block Size
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&minBlockSize),
                reinterpret_cast<const uint8_t*>(&minBlockSize) + sizeof(minBlockSize));

    // Index Filename Length
    uint16_t filenameLength = indexFileName.length();
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&filenameLength),
                reinterpret_cast<const uint8_t*>(&filenameLength) + sizeof(filenameLength));

    // Index Filename
    data.insert(data.end(), indexFileName.begin(), indexFileName.end());

    // Index File Schema Info Length
    uint16_t schemaInfoLength = indexFileSchemaInfo.length();
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&schemaInfoLength),
                reinterpret_cast<const uint8_t*>(&schemaInfoLength) + sizeof(schemaInfoLength));

    // Index File Schema Info
    data.insert(data.end(), indexFileSchemaInfo.begin(), indexFileSchemaInfo.end());

    // Record Count
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&recordCount),
                reinterpret_cast<const uint8_t*>(&recordCount) + sizeof(recordCount));

    // Block Count
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&blockCount),
                reinterpret_cast<const uint8_t*>(&blockCount) + sizeof(blockCount));
    
    // Field Count
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&fieldCount),
                reinterpret_cast<const uint8_t*>(&fieldCount) + sizeof(fieldCount));

    // Fields Array
    for(const auto& field : fields)
    {
        // Length of name field
        uint16_t nameLength = field.name.length();
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&nameLength),
                    reinterpret_cast<const uint8_t*>(&nameLength) + sizeof(nameLength));

        // Name of field
        data.insert(data.end(), field.name.begin(), field.name.end());

        // Field Type
        data.push_back(field.type);
    }

    // Primary Key Field
    data.push_back(primaryKeyField);

    // Available List RBN
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&availableListRBN),
                reinterpret_cast<const uint8_t*>(&availableListRBN) + sizeof(availableListRBN));

    // Sequence Set List RBN
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&sequenceSetListRBN),
                reinterpret_cast<const uint8_t*>(&sequenceSetListRBN) + sizeof(sequenceSetListRBN));

    // Stale Flag
    data.push_back(staleFlag);

    // Calculate Header Size
    uint32_t trueHeaderSize = data.size();
    memcpy(&data[headerSizePos], &trueHeaderSize, sizeof(trueHeaderSize));

    return data;
}

HeaderRecord HeaderRecord::deserialize(const uint8_t* data)
{
   HeaderRecord header;
    size_t offset = 0;

    // Read File Structure Type
    memcpy(header.fileStructureType, data + offset, 4);
    offset += 4;

    // Read Version
    memcpy(&header.version, data + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    // Read Header Size
    memcpy(&header.headerSize, data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Read Size Format Type
    header.sizeFormatType = data[offset++];

    // Read Block Size
    memcpy(&header.blockSize, data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Read Min Block Size
    memcpy(&header.minBlockSize, data + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    // Read Index File Name Size
    uint16_t fileNameSize;
    memcpy(&fileNameSize, data + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    // Read Index File Name
    header.indexFileName = std::string(reinterpret_cast<const char*>(data + offset), fileNameSize);
    offset += fileNameSize;

    // Read Index File Schema Info Size
    uint16_t schemaInfoSize;
    memcpy(&schemaInfoSize, data + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    // Read Schema Info Size
    header.indexFileSchemaInfo = std::string(reinterpret_cast<const char*>(data + offset), schemaInfoSize);
    offset += schemaInfoSize;

    // Read Record Count
    memcpy(&header.recordCount, data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Read Block Count
    memcpy(&header.blockCount, data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Read Field Count
    memcpy(&header.fieldCount, data + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    // Read Fields Array
    header.fields.clear();
    for (uint16_t i = 0; i < header.fieldCount; i++) 
    {
        FieldDef field;
        
        // Read field name length
        uint16_t nameLength;
        memcpy(&nameLength, data + offset, sizeof(uint16_t));
        offset += sizeof(uint16_t);
        
        // Read field name
        field.name = std::string(reinterpret_cast<const char*>(data + offset), nameLength);
        offset += nameLength;
        
        // Read field type
        field.type = data[offset++];
        
        header.fields.push_back(field);
    }

    // Read Primary Key Field
    header.primaryKeyField = data[offset++];

    // Read Avail List RBN
    memcpy(&header.availableListRBN, data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Read Sequence List RBN
    memcpy(&header.sequenceSetListRBN, data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Read Has Valid Index File
    header.staleFlag = data[offset++];

    return header;
}

// GETTERS
const char* HeaderRecord::getFileStructureType() const
{
    return fileStructureType;
}

uint16_t HeaderRecord::getVersion() const
{
    return version;
}

uint32_t HeaderRecord::getHeaderSize() const
{
    return headerSize;
}

uint8_t HeaderRecord::getSizeFormatType() const
{
    return sizeFormatType;
}

uint32_t HeaderRecord::getBlockSize() const
{
    return blockSize;
}

uint16_t HeaderRecord::getMinBlockSize() const
{
    return minBlockSize;
}

const std::string& HeaderRecord::getIndexFileName() const
{
    return indexFileName;
}

const std::string& HeaderRecord::getIndexFileSchemaInfo() const
{
    return indexFileSchemaInfo;
}

uint32_t HeaderRecord::getRecordCount() const
{
    return recordCount;
}

uint32_t HeaderRecord::getBlockCount() const
{
    return blockCount;
}

uint16_t HeaderRecord::getFieldCount() const
{
    return fieldCount;
}

const std::vector<FieldDef>& HeaderRecord::getFields() const
{
    return fields;
}

uint8_t HeaderRecord::getPrimaryKeyField() const
{
    return primaryKeyField;
}

uint32_t HeaderRecord::getAvailableListRBN() const
{
    return availableListRBN;
}

uint32_t HeaderRecord::getSequenceSetListRBN() const
{
    return sequenceSetListRBN;
}

uint8_t HeaderRecord::getStaleFlag() const
{
    return staleFlag;
}

//SETTERS
void HeaderRecord::setFileStructureType(const char* type)
{
    strncpy(fileStructureType, type, 4);
}

void HeaderRecord::setVersion(uint16_t ver)
{
    this->version = ver;
}

void HeaderRecord::setHeaderSize(uint32_t size)
{
    this->headerSize = size;
}

void HeaderRecord::setSizeFormatType(uint8_t type)
{
    this->sizeFormatType = type;
}

void HeaderRecord::setBlockSize(uint32_t size)
{
    this->blockSize = size;
}

void HeaderRecord::setMinBlockSize(uint16_t size)
{
    this->minBlockSize = size;
}

void HeaderRecord::setIndexFileName(const std::string& filename)
{
    this->indexFileName = filename;
}

void HeaderRecord::setIndexFileSchemaInfo(const std::string& schemaInfo)
{
    this->indexFileSchemaInfo = schemaInfo;
}

void HeaderRecord::setRecordCount(uint32_t count)
{
    this->recordCount = count;
}

void HeaderRecord::setRecordCount(uint32_t count)
{
    this->blockCount = count;
}

void HeaderRecord::setFieldCount(uint16_t count)
{
    this->fieldCount = count;
}

void HeaderRecord::setFields(const std::vector<FieldDef>& fieldDefs)
{
    this->fields = fieldDefs;
}

void HeaderRecord::setPrimaryKeyField(uint8_t field)
{
    this->primaryKeyField = field;
}

void HeaderRecord::setAvailableListRBN(uint32_t rbn)
{
    this->availableListRBN = rbn;
}

void HeaderRecord::setSequenceSetListRBN(uint32_t rbn)
{
    this->sequenceSetListRBN = rbn;
}

void HeaderRecord::addField(const FieldDef& field)
{
    this->fields.push_back(field);
    this->fieldCount = fields.size();
}

void HeaderRecord::setStaleFlag(uint8_t isStale)
{
    this->staleFlag = isStale;
}