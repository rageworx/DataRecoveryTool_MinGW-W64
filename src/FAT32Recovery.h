#pragma once
//#include "Structures.h"
#include "FAT32Structs.h"
#include "Utils.h"
#include "SectorReader.h"
//#include "LogicalDriveReader.h"
#include "Enums.h"
#include "ClusterHistory.h"
#include <cstdint>
#include <string>
#include <windows.h>
#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <memory>



namespace fs = std::filesystem;


class FAT32Recovery : public IConfigurable{
private:
    // Cluster validation
    static constexpr const uint32_t MIN_DATA_CLUSTER = 2;  // First valid data cluster
    static constexpr const uint32_t BAD_CLUSTER = 0x0FFFFFF7;
    static constexpr const uint32_t MAX_VALID_CLUSTER = 0x0FFFFFF6;

    // File corruption analysis
    static constexpr uint32_t MINIMUM_CLUSTERS_FOR_ANALYSIS = 10; // 5
    static constexpr uint32_t LARGE_GAP_THRESHOLD = 1000; // 1000
    static constexpr double SUSPICIOUS_PATTERN_THRESHOLD = 0.1; // 10%
    static constexpr double SEVERE_PATTERN_THRESHOLD = 0.25;    // 25%
    static constexpr double FILENAME_CORRUTPION_THRESHOLD = 0.5; // 50% bad chars in name
    ClusterHistory clusterHistory;// used with finding cluster overwrites
    uint32_t nextFileId = 0; // used with finding cluster overwrites

    Utils utils;
    //const Config& config;
    struct DriveInfo {
        BootSector bootSector;
        uint32_t fatStartSector;
        uint32_t dataStartSector;
        uint32_t rootDirCluster;
        uint32_t maxClusterCount;
    } driveInfo;

    uint16_t fileId = 1;
    std::vector<FAT32FileInfo> recoveryList;
    std::unique_ptr<SectorReader> sectorReader;
    DriveType driveType = DriveType::UNKNOWN_TYPE; // not implemented yet

    void printToolHeader() const;

    void setSectorReader(std::unique_ptr<SectorReader> reader);
    bool readSector(uint64_t sector, void* buffer, uint64_t size);
    void readBootSector(uint32_t sector);
    uint64_t getBytesPerSector();

    bool isValidCluster(uint32_t cluster) const;
    uint32_t sanitizeCluster(uint32_t cluster) const;
    uint32_t clusterToSector(uint32_t cluster) const;
    uint32_t getNextCluster(uint32_t cluster);
  
    /*=============== File scan ===============*/
    // Scan drive for deleted files
    void scanForDeletedFiles(uint32_t startSector);
    void scanDirectory(uint32_t cluster, bool isTargetFolder = false);
    void processEntriesInSector(uint32_t entriesPerSector, bool isTargetFolder, std::vector<uint8_t>& sectorBuffer);
    void processDirectoryEntry(const DirectoryEntry* entry, const std::wstring& filename, bool isTargetFolder);
    void addToRecoveryList(const FAT32FileInfo& fileInfo);
    // Extract long filename from LFN entry
    std::wstring getLongFilename(DirectoryEntry* entry) const;
    // Extract short filename from Directory Entry
    std::wstring getShortFilename(const DirectoryEntry* entry, bool isDeleted = false)const;
    // Parse filename into components
    FAT32FileInfo parseFileInfo(const std::wstring& fullName, uint32_t startCluster, uint32_t expectedSize);
    // Compare two filenames (case-insensitive)
    bool compareFolderNames(const std::wstring& filename1, const std::wstring& filename2) const;
    // Predict file extension based on content
    std::wstring predictExtension(const uint32_t cluster, const uint32_t expectedSize);
    // Get file signature from first bytes
    std::string getFileSignature(const std::vector<BYTE>& data) const;
    // Convert file signature to extension
    std::wstring guessFileExtension(const std::string& signature) const;
  

    /*=============== Corruption analysis ===============*/
    // Check if cluster is marked as in use in the FAT
    bool isClusterInUse(uint32_t cluster);
    // Analyzes clusters for repetition, gaps, backward jumps and calculates the fragmentation score
    void analyzeClusterPattern(const std::vector<uint32_t>& clusters, RecoveryStatus& status) const;
    // Checks if a filename is corrupted
    bool isFileNameCorrupted(const std::wstring& filename) const;
    // Find deleted files overwritten by other deleted files
    OverwriteAnalysis analyzeClusterOverwrites(uint32_t startCluster, uint32_t expectedSize);

    /*=============== Recovery ===============*/
    // Asks user to either recover all files or only the selected IDs
    std::vector<FAT32FileInfo> selectFilesToRecover(const std::vector<FAT32FileInfo>& deletedFiles);
    // Recover all found deleted files
    void recoverPartition();
    // Processes each file for recovery based on config options
    void processFileForRecovery(const FAT32FileInfo& fileInfo);
    // Validate cluster chain and find signs of corruption
    void validateClusterChain(RecoveryStatus& status, const uint32_t startCluster, std::vector<uint32_t>& clusterChain, uint32_t expectedSize, const fs::path& outputPath, bool isExtensionPredicted);
    // Recover specific file
    void recoverFile(const std::vector<uint32_t>& clusterChain, RecoveryStatus& status, const fs::path& outputPath, const uint32_t expectedSize);

    /*=============== Recovery and analysis results ===============*/
    void showAnalysisResult(const RecoveryStatus& status) const;
    void showRecoveryResult(const RecoveryStatus& status, const fs::path& outputPath, const uint32_t expectedSize) const;

    // Recovery entry point
    void runLogicalDriveRecovery();

public:
    // Constructor
    explicit FAT32Recovery(const DriveType& driveType, std::unique_ptr<SectorReader> reader);
    ~FAT32Recovery();

    void startRecovery();
};