#pragma once
#include "IConfigurable.h"
#include "Utils.h"
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
    Utils utils;
    const DriveType& driveType;

    std::unique_ptr<SectorReader> sectorReader;
    
    //std::vector<uint8_t> clusterData;
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

    struct DriveInfo {
        ExFATBootSector bootSector;
        uint32_t bytesPerSector;      // From BytesPerSectorShift
        uint32_t sectorsPerCluster;   // From SectorsPerClusterShift
    } driveInfo;



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


    void scanForDeletedFiles();

    void scanDirectory(uint32_t cluster, uint32_t depth = 0);

    void processEntriesInSector(uint32_t entriesPerSector, const std::vector<uint8_t>& sectorBuffer);

    void processDirectoryEntry(const DirectoryEntryCommon* entry, exFATDirEntryData& dirData);

    void finalizeDirectoryEntry(exFATDirEntryData& dirData);

    exFATFileInfo parseFileInfo(const exFATDirEntryData& dirData);

    void addToRecoveryList(const exFATFileInfo& fileInfo);

    void recoverPartition();


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