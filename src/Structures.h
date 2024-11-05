#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#pragma pack(push, 1)
struct BootSector {
    uint8_t  jmpBoot[3];
    uint8_t  OEMName[8];
    uint16_t BytesPerSector;
    uint8_t  SectorsPerCluster;
    uint16_t ReservedSectorCount;
    uint8_t  NumFATs;
    uint16_t RootEntryCount;
    uint16_t TotalSectors16;
    uint8_t  Media;
    uint16_t FATSize16;
    uint16_t SectorsPerTrack;
    uint16_t NumberOfHeads;
    uint32_t HiddenSectors;
    uint32_t TotalSectors32;
    uint32_t FATSize32;
    uint16_t ExtFlags;
    uint16_t FSVersion;
    uint32_t RootCluster;
    uint16_t FSInfo;
    uint16_t BkBootSec;
    uint8_t  Reserved[12];
    uint8_t  DriveNumber;
    uint8_t  Reserved1;
    uint8_t  BootSignature;
    uint32_t VolumeID;
    uint8_t  VolumeLabel[11];
    uint8_t  FileSystemType[8];
    uint8_t BootCode[420];
    uint16_t BootSectorSignature;
};

struct DirectoryEntry {
    uint8_t  Name[11];
    uint8_t  Attr;
    uint8_t  NTRes;
    uint8_t  CrtTimeTenth;
    uint16_t CrtTime;
    uint16_t CrtDate;
    uint16_t LstAccDate;
    uint16_t FstClusHI;
    uint16_t WrtTime;
    uint16_t WrtDate;
    uint16_t FstClusLO;
    uint32_t FileSize;
};

struct LFNEntry {
    uint8_t Ord;
    uint16_t Name1[5];
    uint8_t Attr;
    uint8_t Type;
    uint8_t Chksum;
    uint16_t Name2[6];
    uint16_t FstClusLO;
    uint16_t Name3[2];
};

struct FileInfo {
    uint16_t fileId;
    std::wstring fullName;
    std::wstring fileName;
    std::wstring extension;
    bool isExtensionPredicted;
};

struct Config {
    std::wstring drivePath;
    std::wstring inputFolder;
    std::wstring outputFolder;
    std::wstring logFolder;
    std::wstring logFile;
    uint32_t targetCluster;
    uint32_t targetFileSize;
    bool createFileDataLog;
    bool recover; // not used
    bool analyze;
};

struct MBRPartitionEntry {
    uint8_t BootIndicator; // 0x80 for bootable, 0x00 for non-bootable
    uint8_t StartHead;
    uint8_t StartSector; // lower 6 bits represent sector number
    uint8_t StartCylinder;
    uint8_t Type;      // File system type (e.g., NTFS, FAT32)
    uint8_t EndHead;
    uint8_t EndSector; // lower 6 bits represent sector number
    uint8_t EndCylinder;
    uint32_t StartLBA;     // Starting logical block address
    uint32_t TotalSectors;  // Number of sectors in the partition
};

struct MBRHeader {
    uint8_t bootCode[446];         // Bootstrap code (first 446 bytes)
    MBRPartitionEntry PartitionTable[4];
    uint16_t signature;            // Boot signature (0x55AA)
};


struct GPTHeader {
    uint8_t  Signature[8];         // EFI PART (45 46 49 20 50 41 52 54)
    uint32_t Revision;            // GPT Revision (usually 0x00010000 for version 1.0)
    uint32_t HeaderSize;          // Size of GPT header (usually 92 bytes)
    uint32_t HeaderCRC32;         // CRC32 of header
    uint32_t Reserved;            // Must be zero
    uint64_t CurrentLBA;          // Location of this header copy
    uint64_t BackupLBA;           // Location of the other header copy
    uint64_t FirstUsableLBA;      // First usable LBA for partitions
    uint64_t LastUsableLBA;       // Last usable LBA for partitions
    uint8_t  DiskGUID[16];        // Disk GUID (also known as UUID)
    uint64_t PartitionEntryLBA;   // Starting LBA of partition entries
    uint32_t NumberOfEntries;     // Number of partition entries
    uint32_t SizeOfEntry;         // Size of a partition entry (usually 128)
    uint32_t PartitionEntryArrayCRC32; // CRC32 of partition array
    uint8_t  Reserved2[420];      // Reserved; must be zero for alignment
};

// GPT Partition Entry Structure
struct GPTPartitionEntry {
    uint8_t  PartitionTypeGUID[16];   // Partition type GUID
    uint8_t  UniquePartitionGUID[16]; // Unique partition GUID
    uint64_t StartingLBA;             // Starting LBA
    uint64_t EndingLBA;               // Ending LBA
    uint64_t Attributes;              // Partition attributes
    uint16_t PartitionName[36];       // Partition name (72 bytes UTF-16LE).
};


/* ========= Cluster analysis =========*/
struct ClusterAnalysisResult {
    double fragmentation;      // 0.0-1.0, higher means more fragmented
    bool isCorrupted;
    uint32_t backJumps;
    uint32_t repeatedClusters;
    uint32_t largeGaps;
};

struct RecoveryStatus {
    bool isCorrupted;
    bool hasFragmentedClusters;
    double fragmentation;
    bool hasBackJumps;
    uint32_t backJumps;
    bool hasRepeatedClusters;
    uint32_t repeatedClusters;
    bool hasLargeGaps;
    uint32_t largeGaps;
    bool hasOverwrittenClusters;
    bool hasInvalidFileName;
    bool hasInvalidExtension;
    uint32_t expectedClusters;
    uint32_t recoveredClusters;
    uint32_t recoveredBytes;
    std::vector<uint32_t> problematicClusters;
};

struct ClusterUsage {
    uint64_t timestamp;  // When this cluster was used
    uint32_t fileId;     // Identifier for the deleted file
    bool isDeleted;      // Whether this usage was from a deleted file
    uint64_t writeOffset; // Offset within the file where this cluster was used
};

struct OverwriteAnalysis {
    bool hasOverwrite;
    std::vector<uint32_t> overwrittenClusters;
    std::map<uint32_t, std::vector<uint32_t>> overwrittenBy; // cluster -> list of file IDs that overwrote it
    double overwritePercentage;
};



/* exFAT */

struct ExFATBootSector {
    uint8_t  JumpBoot[3];           // Jump instruction to boot code
    uint8_t  FileSystemName[8];     // "EXFAT   "
    uint8_t  MustBeZero[53];        // Always 0
    uint64_t PartitionOffset;       // Sector address of partition
    uint64_t VolumeLength;          // Size of partition in sectors
    uint32_t FatOffset;             // FAT start sector from partition start
    uint32_t FatLength;             // Size of FAT in sectors
    uint32_t ClusterHeapOffset;     // First cluster sector from partition
    uint32_t ClusterCount;          // Total clusters in volume
    uint32_t RootDirectoryCluster;  // First cluster of root directory
    uint32_t VolumeSerialNumber;    // Volume serial number
    uint16_t FileSystemRevision;    // File system version (usually 0x100)
    uint16_t VolumeFlags;           // Volume state flags
    uint8_t  BytesPerSectorShift;   // Log2 of bytes per sector
    uint8_t  SectorsPerClusterShift;// Log2 of sectors per cluster
    uint8_t  NumberOfFats;          // Number of FATs (usually 1)
    uint8_t  DriveSelect;           // INT 13h drive number
    uint8_t  PercentInUse;          // Percentage of clusters in use
    uint8_t  Reserved[7];           // Reserved for future use
    uint8_t  BootCode[390];         // Boot code and/or message
    uint16_t BootSignature;         // Must be 0xAA55
};

#pragma pack(pop)