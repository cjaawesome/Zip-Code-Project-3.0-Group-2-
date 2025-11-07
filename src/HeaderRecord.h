#ifndef HEADER_RECORD
#define HEADER_RECORD

#include "stdint.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
/**
 * @file HeaderRecord.h
 * @author Group 2
 * @brief HeaderRecord class for storing header information
 * @version 0.1
 * @date 2025-10-02
 */


// Field definition
struct FieldDef 
{
    std::string name; // Name of the field
    uint8_t type;     // 1=int, 2=float, 3=string, etc.
};

/**
 * @class ZipCodeRecord
 * @brief Represents the information in the header of a file
 * @details Stores file structure type, file version, header size, size Format Type, size of sizes, size inclusion flags, index file names, record count, field count, fields and the primary key field
 */
class HeaderRecord
{
public:
    /**
     * @brief Default constructor
     * @details Initializes all fields to default values
     */
    HeaderRecord(); // Default Constructor
    
    ~HeaderRecord();
    /**
     * @brief serialize
     * @details returns the header in binary format
     */
    std::vector<uint8_t> serialize() const;
    /**
     * @brief deserialize
     * @details converts data vetctor into Header record format and returns it
     * @param data serialized header
     * @returns deserialized data in the form of a HeaderRecord
     */
    static HeaderRecord deserialize(const uint8_t* data); // Read from binary format

    
    /**
     * @brief File Structure Type Getter
     * @returns fileStructureType[4]
     */
    const char* getFileStructureType() const;
    /**
     * @brief Version Getter
     * @returns version
     */
    uint16_t getVersion() const;
    /**
     * @brief HeaderSize Getter
     * @returns headerSize
     */
    uint32_t getHeaderSize() const; 
    /**
     * @brief Size Format Type Getter
     * @returns sizeFormatType
     */
    uint8_t getSizeFormatType() const;
    /**
     * @brief Get Index File Name Getter
     * @returns indexFileName
     */
    const std::string& getIndexFileName() const;
    /**
     * @brief Record Count Getter
     * @returns recordCount
     */
    uint32_t getRecordCount() const;

    uint32_t getBlockCount() const;
    /**
     * @brief Field Count Getter
     * @returns fieldCount
     */
    uint16_t getFieldCount() const;
    /**
     * @brief Fields Getter
     * @returns fields
     */
    const std::vector<FieldDef>& getFields() const; 
    /**
     * @brief Primary Key Field Getter
     * @returns primaryKeyField
     */
    uint8_t getPrimaryKeyField() const;
    /**
     * @brief Gets the staleFlag from the header
     * @returns staleFlag
     */
    uint8_t getStaleFlag() const;
    /**
     * @brief Index File Schema Info Getter
     * @returns indexFileSchemaInfo
     */
    const std::string& getIndexFileSchemaInfo() const;
    /**
     * @brief Block Size Getter
     * @returns blockSize
     */
    uint32_t getBlockSize() const;
    /**
     * @brief Minimum Block Size Getter
     * @returns minBlockSize
     */
    uint16_t getMinBlockSize() const;
    /**
     * @brief Available List RBN Getter
     * @returns availableListRBN
     */
    uint32_t getAvailableListRBN() const;
    /**
     * @brief Sequence Set List RBN Getter
     * @returns sequenceSetListRBN
     */
    uint32_t getSequenceSetListRBN() const;
    /**
     * @brief File Structure Type Setter
     * @details sets fileStructureType[4] to type
     * @param Type new File structure type
     */
    void setFileStructureType(const char* type);
    /**
     * @brief Version Setter
     * @details sets version to ver
     * @param ver new version
     */
    void setVersion(uint16_t ver);
    /**
     * @brief Header Size Setter
     * @details sets headerSize to size
     * @param size new headerSize value
     */
    void setHeaderSize(uint32_t size);
    /**
     * @brief Size Format Type Setter
     * @details sets sizeFormatType to type
     * @param type new sizeFormatType value
     */
    void setSizeFormatType(uint8_t type);
    /**
     * @brief Index File Name Setter
     * @details sets indexFileName to filename
     * @param filename new indexFileName value
     */
    void setIndexFileName(const std::string& filename);
    /**
     * @brief Index File Schema Info Setter
     * @details Sets indexxFileSchemaInfo to schemaInfo
     * @param schemaInfo New Schema Info
     */
    void setIndexFileSchemaInfo(const std::string& schemaInfo);
    /**
     * @brief Record Count Setter
     * @details sets recordCount to count
     * @param count new recordCount value
     */
    void setRecordCount(uint32_t count);

    void setBlockCount(uint32_t count);
    /**
     * @brief Field Count Setter
     * @details sets fieldCount to count
     * @param count new fieldCount value
     */
    void setFieldCount(uint16_t count);
    /**
     * @brief Field Setter
     * @details sets fields to fieldDefs
     * @param fieldDefs new fields value
     */
    void setFields(const std::vector<FieldDef>& fieldDefs);
    /**
     * @brief Primary Key Field Setter
     * @details sets primaryKeyField to field
     * @param field new primaryKeyField value
     */
    void setPrimaryKeyField(uint8_t field);
    /**
     * @brief Adds field to header record
     * @details pushs back fields vector with the new field and updates header size
     * @param field new field to be added
     */
    void addField(const FieldDef& field);
    /**
     * @brief Has Valid Index File Setter
     * @details sets hasValidIndexFile to hasValid
     * @param hasValid new hasValidIndexFile value
     */
    void setStaleFlag(uint8_t staleFlag);
    /**
     * @brief Block Size Setter
     * @details sets blockSize to size
     * @param size new blockSize value
     */
    void setBlockSize(uint32_t size);
    /**
     * @brief Minimum Block Size Setter
     * @details sets minBlockSize to size
     * @param size new minBlockSize value
     */
    void setMinBlockSize(uint16_t size);
    /**
     * @brief Available List RBN Setter
     * @details sets availableListRBN to rbn
     * @param rbn new availableListRBN value
     */
    void setAvailableListRBN(uint32_t rbn);
    /**
     * @brief Sequence Set List RBN Setter
     * @details sets sequenceSetListRBN to rbn
     * @param rbn new sequenceSetListRBN value
     */
    void setSequenceSetListRBN(uint32_t rbn);

private:
    char fileStructureType[4]; // "ZIPC"

    uint16_t version; // File format version

    uint32_t headerSize; // Size of the header record

    uint8_t sizeFormatType; // 0 for Ascii 1 for Binary

    uint32_t blockSize; // Size of blocks used in the file

    uint16_t minBlockSize; // Minimum size of blocks used in the file (default to 25%)

    std::string indexFileName; // Name of the key index file

    std::string indexFileSchemaInfo; // Schema information for the index file

    uint32_t recordCount; // Total number of records

    uint32_t blockCount; // Total number of blocks in the file

    uint16_t fieldCount; // Count of fields per record

    std::vector<FieldDef> fields; // Definitions of each field (Name or ID, Type Schema, etc.)

    uint8_t primaryKeyField; // Unique Identifier for each field (IE: 0 = Zip Code, 1 = State, 2 = County, etc.)

    uint32_t availableListRBN; // RBN of the available list

    uint32_t sequenceSetListRBN; // RBN of the sequence set list
   
    uint8_t staleFlag; // Boolean flag that determines if the index file is valid
};

#endif