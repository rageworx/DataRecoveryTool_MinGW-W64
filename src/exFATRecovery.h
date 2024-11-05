#pragma once
#include "SectorReader.h"
#include "Structures.h"
#include "LogicalDriveReader.h"
#include <cstdint>
#include <memory>
#include <vector>
#include <fstream>
//#include <iostream>
class exFATRecovery {
private:
    const Config& config;
    std::unique_ptr<SectorReader> sectorReader;
    std::vector<uint8_t> sectorBuffer;


    uint32_t bytesPerSector;      // From BytesPerSectorShift (usually 512-4096)
    uint32_t sectorsPerCluster;   // From SectorsPerClusterShift (usually 1-128)
    uint32_t fatOffset;           // 32-bit sector offset to FAT
    uint32_t clusterHeapOffset;   // 32-bit sector offset to data area
    uint32_t rootDirectoryCluster;// 32-bit cluster number
    uint32_t clusterCount;        // 32-bit total cluster count
    uint16_t volumeFlags;         // 16-bit flags field
    uint8_t  numberOfFats;        // 8-bit (typically 1)
    uint64_t volumeLength;        // 64-bit total volume size in sectors

    //ExFATBootSector bootSector;
    // Common to all entry types - first byte defines entry type
    struct DirectoryEntryCommon {
        uint8_t EntryType;      // Type and status of this entry
        uint8_t CustomDefined[19];
        uint32_t FirstCluster;  // First cluster of the file
        uint64_t DataLength;    // Length of the file data
    };

    // Type 0x85: Directory Entry
    struct DirectoryEntry {
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

    void initializeSectorReader();

    void readBootSector(uint32_t sector);

    // Read data from specified sector
    bool readSector(uint32_t sector, void* buffer, uint32_t size);

public:
    exFATRecovery(const Config& cfg);

    // Set the sector reader implementation
    void setSectorReader(std::unique_ptr<SectorReader> reader);

    ~exFATRecovery();
};