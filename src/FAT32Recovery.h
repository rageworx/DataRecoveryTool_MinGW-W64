#pragma once
#include "Structures.h"
#include "SectorReader.h"
#include "LogicalDriveReader.h"
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


class FAT32Recovery{
private:
    //HANDLE hDrive;
    const Config& config;
    BootSector bootSector;
    uint32_t fatStartSector;
    uint32_t dataStartSector;
    uint32_t rootDirCluster;
    uint32_t maxClusterCount;
    std::unique_ptr<SectorReader> sectorReader;
    std::vector<uint8_t> sectorBuffer;
    std::vector<std::pair<FileInfo, DirectoryEntry>> deletedFiles;
    uint16_t fileId = 1; // stored in FileInfo.fileId to have the option to select certain ids

    std::wofstream logFile;
    ClusterHistory clusterHistory;// used with finding cluster overwrites
    uint32_t nextFileId = 0; // used with finding cluster overwrites
    
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

    /*=============== Cluster and Sector Operations ===============*/
    // Read data from specified sector
    bool readSector(uint32_t sector, void* buffer, uint32_t size);
    // Convert cluster number to sector number
    uint32_t clusterToSector(uint32_t cluster) const;
    // Get next cluster in the chain from FAT
    uint32_t getNextCluster(uint32_t cluster);
    // Check if cluster number is within valid range for FAT32
    bool isValidCluster(uint32_t cluster) const;
    uint32_t sanitizeCluster(uint32_t cluster) const;
   

    /*=============== Boot Sector Operations ===============*/
    // Read and parse the boot sector
    void readBootSector(uint32_t sector);
    // Debug: Print boot sector contents in hex
    void printHexArray(const uint8_t* array, size_t size) const;
    // Debug: Print parsed boot sector information
    void printBootSector() const;

    /*=============== Directory Scanning ===============*/
    // Scan directory starting at specified cluster
    void scanDirectory(uint32_t cluster, bool isTargetFolder = false);
    // Process all directory entries in current sector
    void processEntriesInSector(uint32_t entriesPerSector, bool isTargetFolder);
    // Process individual directory entry
    void processDirectoryEntry(DirectoryEntry* entry, const std::wstring& filename, bool isTargetFolder);
    // Add FileInfo and Directory Entry into deletedFiles vector
    void addToDeletedFiles(const DirectoryEntry* entry, const FileInfo& fileInfo);




    /*=============== Folder and File Operations ===============*/
    // Extract long filename from LFN entry
    std::wstring getLongFilename(DirectoryEntry* entry) const;
    // Extract short filename from Directory Entry
    std::wstring getShortFilename(const DirectoryEntry* entry, bool isDeleted = false)const;
    // Parse filename into components
    FileInfo parseFilename(const std::wstring& fullName, uint32_t startCluster, uint32_t expectedSize);
    // Compare two filenames (case-insensitive)
    bool compareFolderNames(const std::wstring& filename1, const std::wstring& filename2) const;

    // Predict file extension based on content
    std::wstring predictExtension(const uint32_t cluster, const uint32_t expectedSize);
    // Get file signature from first bytes
    std::string getFileSignature(const std::vector<BYTE>& data) const;
    // Convert file signature to extension
    std::wstring guessFileExtension(const std::string& signature) const;
   

    /*=============== Utility Functions ===============*/
    // Check if folder exists
    bool folderExists(const fs::path& folderPath) const;
    // Creates the recovery folder if it does not exist
    void createFolderIfNotExists(const fs::path& folderPath) const;
    // Function to print a header for each stage
    void printHeader(const std::string& stage, char borderChar = '_', int width = 60) const;
    // Function to print a footer divider between stages
    void printFooter(char dividerChar = '_', int width = 60) const;
    void printItemDivider(char dividerChar = '-', int width = 60) const;
    // Display progress indicator
    void showProgress(uint64_t currentValue, uint64_t maxValue) const;
    // Generate output path for recovered file
    fs::path getOutputPath(const std::wstring& fullName, const std::wstring& folder) const;

    /*=============== Corruption Analysis Functions ===============*/
    // Check if cluster is marked as in use in the FAT
    bool isClusterInUse(uint32_t cluster);
    // Analyzes clusters for repetition, gaps, backward jumps and calculates the fragmentation score
    void analyzeClusterPattern(const std::vector<uint32_t>& clusters, RecoveryStatus& status) const;
    // Checks if a filename is corrupted
    bool isFileNameCorrupted(const std::wstring& filename) const;
    // Find deleted files overwritten by other deleted files
    OverwriteAnalysis analyzeClusterOverwrites(uint32_t startCluster, uint32_t expectedSize);

    /*=============== Recovery Functions ===============*/
    // Initializes a log file for saving file location data, if enabled
    void initializeFileDataLog(const fs::path& outputPath);
    // Writes file data to a log (File name, file size, cluster and whether an extension was predicted)
    void writeToFileDataLog(const FileInfo& fileInfo, uint32_t cluster, uint32_t fileSize);
    // Asks user if they want to proceed without location file
    bool confirmProceedWithoutLogFile() const;
    // Asks user to either recover all files or only the selected IDs
    std::vector<std::pair<FileInfo, DirectoryEntry>> selectFilesToRecover(const std::vector<std::pair<FileInfo, DirectoryEntry>>& deletedFiles);
    // Processes each file for recovery based on config options
    void processFileForRecovery(const std::pair<FileInfo, DirectoryEntry>& file);
    // Validate cluster chain and find signs of corruption
    void validateClusterChain(RecoveryStatus& status, const uint32_t startCluster, std::vector<uint32_t>& clusterChain, uint32_t expectedSize, const fs::path& outputPath, bool isExtensionPredicted);
    // Recover specific file
    void recoverFile(const std::vector<uint32_t>& clusterChain, RecoveryStatus& status, const fs::path& outputPath, const uint32_t expectedSize);
    // Shows the results of file corruption analysis process
    void showAnalysisResult(const RecoveryStatus& status) const;
    // Show the results of file recovery process
    void showRecoveryResult(const RecoveryStatus& status, const fs::path& outputPath, const uint32_t expectedSize) const;
public:
    // Constructor
    explicit FAT32Recovery(const Config& config);

    /*=============== Core Public Interface ===============*/
    // Set the sector reader implementation
    void setSectorReader(std::unique_ptr<SectorReader> reader);
    // Scan drive for deleted files
    void scanForDeletedFiles(uint32_t startSector);
    // Recover all found deleted files
    void recoverAllFiles();

};