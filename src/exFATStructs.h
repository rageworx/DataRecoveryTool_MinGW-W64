#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct exFATFileInfo {
    uint16_t fileId;
    std::wstring fileName;
    uint64_t fileSize;
    uint32_t cluster;
};

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
// Common to all entry types - first byte defines entry type
struct DirectoryEntryCommon {
    uint8_t EntryType;      // Type and status of this entry
    uint8_t CustomDefined[19];
    uint32_t FirstCluster;  // First cluster of the file
    uint64_t DataLength;    // Length of the file data
};

struct DirectoryEntryExFAT {
    uint8_t EntryType;      // Must be 0x85
    uint8_t SecondaryCount; // Count of secondary entries (stream entry + name entries)
    uint16_t SetChecksum;   // Checksum of the directory entry set
    uint16_t FileAttributes;// File attributes (similar to FAT32)
    uint16_t Reserved1;
    uint32_t CreateTimestamp;
    uint32_t LastModifiedTimestamp;
    uint32_t LastAccessTimestamp;
    uint8_t Create10msIncrement;     // 10ms increment for create time
    uint8_t LastModified10msIncrement;// 10ms increment for modify time
    uint8_t CreateUtcOffset;         // UTC offset for create time
    uint8_t LastModifiedUtcOffset;   // UTC offset for modify time
    uint8_t LastAccessUtcOffset;     // UTC offset for access time
    uint8_t Reserved2[7];
};


// Type 0xC0: Stream Extension Entry
struct StreamExtensionEntry {
    uint8_t EntryType;      // Must be 0xC0
    uint8_t GeneralFlags;   // Flags (e.g., AllocationPossible, NoFatChain)
    uint8_t Reserved1;
    uint8_t NameLength;     // Length of filename in Unicode chars
    uint16_t NameHash;      // Hash of the filename
    uint16_t Reserved2;
    uint64_t ValidDataLength; // Valid data length (may be less than DataLength)
    uint32_t Reserved3;
    uint32_t FirstCluster;    // First cluster of file data
    uint64_t DataLength;      // Total length of file data
};

// Type 0xC1: File Name Entry
struct FileNameEntry {
    uint8_t EntryType;      // Must be 0xC1
    uint8_t GeneralFlags;   // Flags (usually 0)
    uint16_t FileName[15];  // Part of the Unicode filename (up to 15 chars)
};

struct exFATDirEntryData {
    std::wstring longFilename;
    uint32_t startingCluster;
    uint64_t fileSize;
    bool inFileEntry;
    bool isDirectory;
    bool isDeleted;
};

#pragma pack(pop)