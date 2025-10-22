// DataManager.cpp
#include "DataManager.h"
#include <sstream>
#include <map>
#include <iostream>

void DataManager::updateExtremes(Extremes& ex, const ZipCodeRecord& rec) 
{
    if (!ex.initialized) 
    {
        ex.easternmost = rec;
        ex.westernmost = rec;
        ex.northernmost = rec;
        ex.southernmost = rec;
        ex.initialized = true;
        return;
    }
    if (rec.isEastOf(ex.easternmost))   ex.easternmost = rec;
    if (rec.isWestOf(ex.westernmost))   ex.westernmost = rec;
    if (rec.isNorthOf(ex.northernmost)) ex.northernmost = rec;
    if (rec.isSouthOf(ex.southernmost)) ex.southernmost = rec;
}

void DataManager::processRecord(const ZipCodeRecord& rec) 
{
    const char* st = rec.getState();
    if (!st || st[0] == '\0') return;
    
    // Enforce two-char state IDs
    if (st[0] == '\0' || st[1] == '\0' || st[2] != '\0') return;
    
    Extremes& ex = stateExtremes_[std::string(st)];
    updateExtremes(ex, rec);
}

std::size_t DataManager::processFromCsv(const std::string& csvPath) 
{
    stateExtremes_.clear();
    
    CSVBuffer buf;
    if (!buf.openFile(csvPath)) 
    {
        std::ostringstream oss;
        oss << "Failed to open CSV \"" << csvPath << "\"";
        if (buf.hasError()) oss << " - " << buf.getLastError();
        throw std::runtime_error(oss.str());
    }
    
    std::size_t processed = 0;
    ZipCodeRecord rec;
    
    // Stream through records
    while (buf.hasMoreRecords()) 
    {
        if (buf.getNextRecord(rec)) 
        {
            processRecord(rec);
            ++processed;
        } else if (buf.hasError()) 
        {
            std::cerr << "Parse error on line " << buf.getCurrentLineNumber()
                      << ": " << buf.getLastError() << "\n";
        }
    }
    
    buf.closeFile();
    
    if (processed == 0) 
    {
        throw std::runtime_error("No valid records processed from: " + csvPath);
    }
    return processed;
}

std::size_t DataManager::processFromLengthIndicated(const std::string& zcdPath) 
{
    stateExtremes_.clear();
    
    // Read header first
    HeaderBuffer headerBuf;
    HeaderRecord header;
    if (!headerBuf.readHeader(zcdPath, header)) {
        std::ostringstream oss;
        oss << "Failed to read header from \"" << zcdPath << "\"";
        if (headerBuf.hasError()) oss << " - " << headerBuf.getLastError();
        throw std::runtime_error(oss.str());
    }
    
    // Open data buffer
    CSVBuffer buf;
    if (!buf.openLengthIndicatedFile(zcdPath, header.getHeaderSize())) 
    {
        std::ostringstream oss;
        oss << "Failed to open length-indicated file \"" << zcdPath << "\"";
        if (buf.hasError()) oss << " - " << buf.getLastError();
        throw std::runtime_error(oss.str());
    }
    
    std::size_t processed = 0;
    ZipCodeRecord rec;
    
    // Stream through records
    while (buf.getNextLengthIndicatedRecord(rec)) 
    {
        processRecord(rec);
        ++processed;
    }
    
    buf.closeFile();
    
    if (processed == 0) 
    {
        throw std::runtime_error("No valid records processed from: " + zcdPath);
    }
    return processed;
}

void DataManager::printTable(std::ostream& os) const 
{
    os << "State, EasternmostZIP, WesternmostZIP, NorthernmostZIP, SouthernmostZIP\n";
    
    std::map<std::string, Extremes> sorted(stateExtremes_.begin(), stateExtremes_.end());
    for (const auto& kv : sorted) {
        const std::string& state = kv.first;
        const Extremes& ex = kv.second;
        if (!ex.initialized) continue;
        
        os << state << ", "
           << ex.easternmost.getZipCode()  << ", "
           << ex.westernmost.getZipCode()  << ", "
           << ex.northernmost.getZipCode() << ", "
           << ex.southernmost.getZipCode() << "\n";
    }
}

std::string DataManager::signature() const 
{
    std::ostringstream oss;
    std::map<std::string, Extremes> sorted(stateExtremes_.begin(), stateExtremes_.end());
    for (const auto& kv : sorted) 
    {
        const std::string& state = kv.first;
        const Extremes& ex = kv.second;
        if (!ex.initialized) continue;
        oss << state << ":"
            << ex.easternmost.getZipCode()  << "|"
            << ex.westernmost.getZipCode()  << "|"
            << ex.northernmost.getZipCode() << "|"
            << ex.southernmost.getZipCode() << "\n";
    }
    return oss.str();
}

std::string DataManager::signatureFromCsv(const std::string& csvPath) 
{
    DataManager mgr;
    mgr.processFromCsv(csvPath);
    return mgr.signature();
}

std::string DataManager::signatureFromLengthIndicated(const std::string& zcdPath) 
{
    DataManager mgr;
    mgr.processFromLengthIndicated(zcdPath);
    return mgr.signature();
}

bool DataManager::verifyIdenticalResults(const std::string& fileA,
                                         const std::string& fileB,
                                         bool fileAIsLengthIndicated,
                                         bool fileBIsLengthIndicated) 
{
    std::string sigA = fileAIsLengthIndicated ? 
        signatureFromLengthIndicated(fileA) : signatureFromCsv(fileA);
    std::string sigB = fileBIsLengthIndicated ? 
        signatureFromLengthIndicated(fileB) : signatureFromCsv(fileB);
    return sigA == sigB;
}