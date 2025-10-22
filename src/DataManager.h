// DataManager.h
#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <string>
#include <unordered_map>
#include <iosfwd>
#include "ZipCodeRecord.h"
#include "CSVBuffer.h"
#include "HeaderBuffer.h"
#include "HeaderRecord.h"

/**
 * @file DataManager.h
 * @brief Orchestrates streaming ZIP data and producing per-state extremes.
 *
 * Output table columns (in this order):
 *   State, EasternmostZIP, WesternmostZIP, NorthernmostZIP, SouthernmostZIP
 * Rows are sorted alphabetically by 2-letter state ID.
 */
class DataManager {
public:
    struct Extremes {
        ZipCodeRecord easternmost;
        ZipCodeRecord westernmost;
        ZipCodeRecord northernmost;
        ZipCodeRecord southernmost;
        bool initialized = false;
    };

    DataManager() = default;

    /**
     * @brief Stream records from CSV using CSVBuffer, computing extremes on-the-fly.
     * @param csvPath path to CSV file
     * @return number of records processed
     * @throws std::runtime_error if file open fails or zero records loaded
     */
    std::size_t processFromCsv(const std::string& csvPath);

    /**
     * @brief Stream records from length-indicated file, computing extremes on-the-fly.
     * @param zcdPath path to .zcd file
     * @return number of records processed
     * @throws std::runtime_error if file open fails or zero records loaded
     */
    std::size_t processFromLengthIndicated(const std::string& zcdPath);

    /**
     * @brief Print header + per-state rows to the provided stream.
     * @param os output stream (e.g., std::cout)
     */
    void printTable(std::ostream& os) const;

    /**
     * @brief Canonical signature for equality checking across inputs.
     * @details Returns a deterministic string of "STATE:E|W|N|S" lines
     *          sorted by state; useful to verify identical results across
     *          differently sorted CSV inputs.
     */
    std::string signature() const;

    /**
     * @brief Convenience: process CSV, return signature.
     */
    static std::string signatureFromCsv(const std::string& csvPath);

    /**
     * @brief Convenience: process length-indicated file, return signature.
     */
    static std::string signatureFromLengthIndicated(const std::string& zcdPath);

    /**
     * @brief Verify identical results when using two differently sorted files.
     * @return true if the signatures match; false otherwise
     */
    static bool verifyIdenticalResults(const std::string& fileA,
                                       const std::string& fileB,
                                       bool fileAIsLengthIndicated = false,
                                       bool fileBIsLengthIndicated = false);

private:
    std::unordered_map<std::string, Extremes> stateExtremes_;
    
    /**
     * @brief Process a single record into the extremes map
     * @param rec ZipCodeRecord being processed
     */
    void processRecord(const ZipCodeRecord& rec);
    
    /**
     * @brief Updates the extremes for a state
     * @param ex the extremes being updated
     * @param rec ZipCodeRecord being processed
     */
    static void updateExtremes(Extremes& ex, const ZipCodeRecord& rec);
};
#endif