#pragma once
#include <cstdint>
#include <string>
#include <vector>
#pragma pack(push, 1)
struct FAT32FileInfo {
    uint16_t fileId;
    std::wstring fullName;
    std::wstring fileName;
    std::wstring extension;
    uint64_t fileSize;
    uint32_t cluster;
    bool isExtensionPredicted;
};

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

struct FAT32RecoveryStatus {
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
    uint64_t expectedClusters;
    uint64_t recoveredClusters;
    uint64_t recoveredBytes;
    std::vector<uint64_t> problematicClusters;
};

#pragma pack(pop)