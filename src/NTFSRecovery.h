#pragma once
#include "IConfigurable.h"
#include "NTFSStructs.h"
#include "LogicalDriveReader.h"
#include "SectorReader.h"
#include "Enums.h"
#include "ClusterHistory.h"
#include <cstdint>
#include <memory>
#include <vector>
#include <fstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

class NTFSRecovery : public IConfigurable{
private:
    //const Config& config;
    const DriveType& driveType;
    std::wofstream logFile;

    std::unique_ptr<SectorReader> sectorReader;
    NTFSBootSector bootSector;
    //std::vector<uint8_t> sectorBuffer;

    uint32_t mftRecordSize;
    uint32_t bytesPerCluster;
    uint64_t mftOffset;

    uint16_t fileId = 1;
    std::vector<NTFSFileInfo> recoveryList;

    uint32_t currentRecursionDepth = 0;
    static constexpr uint32_t MAX_RECURSION_DEPTH = 100;

    /* Utils */
    bool folderExists(const fs::path& folderPath) const;
    void createFolderIfNotExists(const fs::path& folderPath) const;
    fs::path getOutputPath(const std::wstring& fullName, const std::wstring& folder) const;
    void showProgress(uint64_t currentValue, uint64_t maxValue) const;

    void initializeLogFile();
    void writeToLogFile(const NTFSFileInfo& fileInfo);
    void closeLogFile();

    void printToolHeader() const;
    void printHeader(const std::string& stage, char borderChar = '_', int width = 60) const;
    void printFooter(char dividerChar = '_', int width = 60) const;
    void printItemDivider(char dividerChar = '-', int width = 60) const;

    // Set the sector reader implementation
    void setSectorReader(std::unique_ptr<SectorReader> reader);
    uint64_t getBytesPerSector();
    uint64_t getTotalMftRecords();
    // Read data from specified sector
    bool readSector(uint64_t sector, void* buffer, uint64_t size);

    void readBootSector(uint64_t sector);

    uint64_t clusterToSector(uint64_t cluster);

    /* Search for deleted files */

    uint32_t getSectorsPerMftRecord();

    bool isValidSector(uint64_t mftSector) const;

    bool isValidFileRecord(const MFTEntryHeader* entry) const;

    bool validateFileInfo(const NTFSFileInfo& fileInfo) const;

    void logFileInfo(const NTFSFileInfo& fileInfo);

    void addToRecoveryList(const NTFSFileInfo& fileInfo);

    void clearFileInfo(NTFSFileInfo& fileInfo) const;

    void scanForDeletedFiles();

    void scanMFT();

    void processMftRecord(std::vector<uint8_t>& mftBuffer);

    bool readMftRecord(std::vector<uint8_t>& mftBuffer, const uint32_t sectorsPerMftRecord, const uint64_t currentSector);

    void processAttribute(const std::vector<uint8_t>& mftBuffer, NTFSFileInfo& fileInfo, uint32_t attributeOffset, bool& hasFileName, bool& hasData, bool isDeleted);

    void processFileNameAttribute(const AttributeHeader* attr, const uint8_t* attrData, bool isDeleted, NTFSFileInfo& fileInfo);

    void processDataAttribute(const AttributeHeader* attr, const uint8_t* attrData, bool isDeleted, NTFSFileInfo& fileInfo);

    /* Recover files */

    std::vector<NTFSFileInfo> selectFilesToRecover(const std::vector<NTFSFileInfo>& recoveryList);

    void runLogicalDriveRecovery();

    void recoverPartition();

    void processFileForRecovery(const NTFSFileInfo& fileInfo);

    void recoverResidentFile(const NTFSFileInfo& fileInfo, const fs::path& outputPath);

    std::vector<uint64_t> validateClusterChain(RecoveryStatus& status, const NTFSFileInfo& fileInfo, uint64_t expectedSize, const fs::path& outputPath, bool isExtensionPredicted);

    void recoverNonResidentFile(const std::vector<uint64_t>& clusterChain, RecoveryStatus& status, const fs::path& outputPath, const uint64_t expectedSize);

    void showRecoveryResult(const fs::path& outputPath) const;

public:
    NTFSRecovery(const DriveType& driveType, std::unique_ptr<SectorReader> reader);

    void startRecovery();
};