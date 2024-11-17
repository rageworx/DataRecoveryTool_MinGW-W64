#pragma once
#include "IConfigurable.h"
#include "SectorReader.h"
#include "Structures.h"
#include "exFATStructs.h"
#include "LogicalDriveReader.h"
#include "Enums.h"
#include "ClusterHistory.h"
#include <cstdint>
#include <memory>
#include <vector>
#include <fstream>
#include <filesystem>
//#include <iostream>

namespace fs = std::filesystem;

class exFATRecovery : public IConfigurable{
private:
    //const Config& config;
    const DriveType& driveType;

    std::wofstream logFile;
    std::unique_ptr<SectorReader> sectorReader;
    std::vector<uint8_t> sectorBuffer;
    std::vector<uint8_t> clusterData;
    std::vector<exFATFileInfo> recoveryList;

    uint16_t fileId = 1;


    // File corruption analysis
    static constexpr uint32_t MINIMUM_CLUSTERS_FOR_ANALYSIS = 10; // 5
    static constexpr uint32_t LARGE_GAP_THRESHOLD = 1000; // 1000
    static constexpr double SUSPICIOUS_PATTERN_THRESHOLD = 0.1; // 10%
    static constexpr double SEVERE_PATTERN_THRESHOLD = 0.25;    // 25%
    static constexpr double FILENAME_CORRUTPION_THRESHOLD = 0.5; // 50% bad chars in name
    ClusterHistory clusterHistory;
    uint32_t nextFileId = 0;

    static constexpr uint32_t MIN_DATA_CLUSTER = 2;         // First valid data cluster for exFAT
    static constexpr uint32_t BAD_CLUSTER = 0xFFFFFFF7;     // exFAT bad cluster marker
    static constexpr uint32_t END_OF_CHAIN = 0xFFFFFFFF;    // exFAT end of chain marker

    uint32_t currentRecursionDepth = 0;
    static constexpr uint32_t MAX_RECURSION_DEPTH = 100;

    ExFATBootSector bootSector;
    uint32_t bytesPerSector = 0;      // From BytesPerSectorShift (usually 512-4096)
    uint32_t sectorsPerCluster = 0;   // From SectorsPerClusterShift (usually 1-128)
    uint32_t fatOffset = 0;           // 32-bit sector offset to FAT
    uint32_t clusterHeapOffset;   // 32-bit sector offset to data area
    uint32_t rootDirectoryCluster;// 32-bit cluster number
    uint32_t clusterCount;        // 32-bit total cluster count
    uint16_t volumeFlags;         // 16-bit flags field
    uint8_t  numberOfFats;        // 8-bit (typically 1)
    uint64_t volumeLength;        // 64-bit total volume size in sectors



    // Bit flags for GeneralFlags in StreamExtensionEntry
    enum StreamFlags {
        ALLOCATION_POSSIBLE = 0x01,
        NO_FAT_CHAIN = 0x02
    };

    // Bit flags for FileAttributes in DirectoryEntry
    enum FileAttributes {
        READ_ONLY = 0x0001,
        HIDDEN = 0x0002,
        SYSTEM = 0x0004,
        DIRECTORY = 0x0010,
        ARCHIVE = 0x0020
    };

    // Helper functions for entry type checking
    inline bool IsDirectoryEntry(uint8_t entryType);

    inline bool IsStreamExtensionEntry(uint8_t entryType);

    inline bool IsFileNameEntry(uint8_t entryType);

    inline bool IsEntryInUse(uint8_t entryType);

    bool isValidCluster(uint32_t cluster) const;

    uint32_t clusterToSector(uint32_t cluster);

    uint32_t getNextCluster(uint32_t cluster);

    void readBootSector(uint32_t sector);

    // Set the sector reader implementation
    void setSectorReader(std::unique_ptr<SectorReader> reader);

    // Read data from specified sector
    bool readSector(uint64_t sector, void* buffer, uint64_t size);

    uint64_t getBytesPerSector();

    std::wstring extractFileName(const FileNameEntry* fnEntry) const;

    bool isValidDeletedEntry(uint32_t cluster, uint64_t size);

    void printToolHeader() const;
    void printHeader(const std::string& stage, char borderChar = '_', int width = 60) const;
    void printFooter(char dividerChar = '_', int width = 60) const;
    void printItemDivider(char dividerChar = '-', int width = 60) const;

    void scanForDeletedFiles();

    void scanDirectory(uint32_t cluster, uint32_t depth = 0);

    void processEntriesInSector(uint32_t entriesPerSector, const std::vector<uint8_t>& sectorBuffer);

    void processDirectoryEntry(const DirectoryEntryCommon* entry, exFATDirEntryData& dirData);

    void finalizeDirectoryEntry(exFATDirEntryData& dirData);

    exFATFileInfo parseFileInfo(const exFATDirEntryData& dirData);

    void addToRecoveryList(const exFATFileInfo& fileInfo);

    void logFileInfo(const exFATFileInfo& fileInfo);


    void recoverPartition();

    /* Utils */
    bool folderExists(const fs::path& folderPath) const;
    // Creates the recovery folder if it does not exist
    void createFolderIfNotExists(const fs::path& folderPath) const;
    
    fs::path getOutputPath(const std::wstring& fullName, const std::wstring& folder) const;

    void showProgress(uint64_t currentValue, uint64_t maxValue) const;

    /*=============== File Log Operations ===============*/
// Creates a log file for saving file location data, if enabled
    void initializeLogFile();
    // Writes file data to a log (File name, file size, cluster and whether an extension was predicted)
    void writeToLogFile(const exFATFileInfo& fileInfo);

    void closeLogFile();

    bool isClusterInUse(uint32_t cluster);
    void analyzeClusterPattern(const std::vector<uint32_t>& clusters, RecoveryStatus& status) const;
    bool isFileNameCorrupted(const std::wstring& filename) const;
    OverwriteAnalysis analyzeClusterOverwrites(uint32_t startCluster, uint32_t expectedSize);

    std::vector<exFATFileInfo> selectFilesToRecover(const std::vector<exFATFileInfo>& recoveryList);

    void runLogicalDriveRecovery();

    void processFileForRecovery(const exFATFileInfo& fileInfo);

    void validateClusterChain(RecoveryStatus& status, const uint32_t startCluster, std::vector<uint32_t>& clusterChain, uint64_t expectedSize, const fs::path& outputPath, bool isExtensionPredicted);

    void recoverFile(const std::vector<uint32_t>& clusterChain, RecoveryStatus& status, const fs::path& outputPath, const uint64_t expectedSize);

    void showRecoveryResult(const RecoveryStatus& status, const fs::path& outputPath, const uint64_t expectedSize) const;

    void showAnalysisResult(const RecoveryStatus& status) const;

public:
    exFATRecovery(const DriveType& driveType, std::unique_ptr<SectorReader> reader);
    ~exFATRecovery();



    void startRecovery();
};