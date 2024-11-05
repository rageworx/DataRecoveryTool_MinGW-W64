#pragma once
#include "Structures.h"
#include "Enums.h"
#include "SectorReader.h"

#include <memory>
#include <cstdint>
#include <vector>


class DriveHandler {
private:
    Config config;
    MBRHeader mbr;
    GPTHeader gpt;
    BootSector bootSector;
    std::unique_ptr<SectorReader> sectorReader;

    uint32_t bytesPerSector; // from DeviceIoControl


    std::vector<MBRPartitionEntry> partitionsMBR;
    std::vector<GPTPartitionEntry> partitionsGPT;

    uint32_t fatStartSector;
    uint32_t dataStartSector;
    uint32_t rootDirCluster;

    DriveType driveType = DriveType::UNKNOWN_TYPE;
    FilesystemType fsType = FilesystemType::UNKNOWN_TYPE;
    PartitionType partitionType = PartitionType::UNKNOWN_TYPE;

    static constexpr uint8_t GUID_FAT32_TYPE[16] = { 0xA2,0xA0, 0xD0, 0xEB, 0xE5, 0xB9, 0x33, 0x44, 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7 };

    // FS type
    static constexpr int FAT32_IDENTIFIER_OFFSET = 0x52;  // FAT32 identifier offset
    static constexpr int NTFS_IDENTIFIER_OFFSET = 0x03;   // NTFS identifier offset
    static constexpr int EXFAT_IDENTIFIER_OFFSET = 0x03;  // exFAT identifier offset

    // Partition type

    static constexpr int MBR_SIGNATURE_OFFSET = 0x1FE;
    static constexpr int GPT_SIGNATURE_OFFSET = 0x00;


    /*=============== Filesystem type identification ===============*/
    // Logical
    inline bool isFat32(const uint8_t* buffer);
    inline bool isExFat(const uint8_t* buffer);
    inline bool isNtfs(const uint8_t* buffer);

    FilesystemType getFilesystemType();

    /*=============== Partition type identification ===============*/
    bool isGpt(const uint8_t* buffer);
    bool isMbr(const uint8_t* buffer);
    PartitionType getPartitionType();

    /*=============== Core Drive Operations ===============*/
       // Initialize sector reader based on drive type
    void initializeSectorReader();

    // Read data from specified sector
    void readSector(uint64_t sector, void* buffer, uint32_t size);
    // Set the sector reader implementation 
    void setSectorReader(std::unique_ptr<SectorReader> reader);
    // Get bytes per sector for the drive
    void getBytesPerSector();
    // Close drive handle before recovery
    void closeDrive();

    /*=============== Drive and Partition Analysis ===============*/
    // Determine if drive is logical or physical
    DriveType determineDriveType(const std::wstring& drivePath);
    // Get partition scheme (MBR or GPT)
    //PartitionScheme getPartitionScheme();
    // Read and parse boot sector
    void readBootSector(uint32_t sector);

    /*=============== MBR Partition Handling ===============*/
    // Read Master Boot Record
    void readMBR();
    // Check if drive uses MBR
    bool isMBR();
    // Get list of MBR partitions
    void getMBRPartitions();

    /*=============== GPT Partition Handling ===============*/
    // Read GUID Partition Table
    void readGPT();
    // Check if drive uses GPT
    bool isGPT();
    // Get list of GPT partitions
    void getGPTPartitions();

    /*=============== Filesystem Detection ===============*/
    // Detect filesystem from boot sector
    FilesystemType getFSTypeFromBootSector(uint32_t sector);
    // Detect filesystem from MBR partition type
    FilesystemType getFSTypeFromMBRPartition(uint8_t type);
    // Detect filesystem from GPT partition GUID
    FilesystemType getFSTypeFromGPTPartition(const uint8_t partitionTypeGUID[16]);

    /*=============== Recovery Methods ===============*/
    // Recover files from logical FAT32 drive
    void recoverFromLogicalDriveFAT32();
    // Entry point for physical drive FAT32 recovery
    void recoverFromPhysicalDriveFAT32();
    // Recover FAT32 files from MBR partition
    void recoverFromPhysicalDriveFAT32MBR(const MBRPartitionEntry& partition);
    // Recover FAT32 files from GPT partition
    void recoverFromPhysicalDriveFAT32GPT(const GPTPartitionEntry& partition);

    /*=============== Debug Output Methods ===============*/
    // Print MBR contents
    void printMBR();
    // Print single MBR partition entry
    void printMBRPartitionEntry(const MBRPartitionEntry& entry, int index);
    // Print array in hex format
    void printHexArray(const uint8_t* array, size_t size);
    // Print boot sector contents
    void printBootSector();
    // Print GPT header information
    void printGPTHeader();
    // Print single GPT partition entry
    void printGPTEntry(const GPTPartitionEntry& entry);

    /*=============== String Conversion Utilities ===============*/
    // Convert GUID to string format
    std::string guidToString(const uint8_t guid[16]);
    // Convert UTF-16 partition name to string
    std::string utf16ToString(const uint16_t utf16Str[], size_t size);

public:
    /*=============== Public Interface ===============*/
    // Constructor
    explicit DriveHandler(const Config& cfg);
    // Main recovery entry point
    void recoverDrive();

    std::unique_ptr<SectorReader> releaseSectorReader() {
        return std::move(sectorReader);
    }
};