#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#pragma pack(push, 1)

struct NTFSFileInfo {
    std::wstring fileName;
    uint16_t fileId;
    uint64_t fileSize;
    uint64_t cluster; // non-resident
    uint64_t runLength; // non-resident
    std::vector<uint8_t> data; // resident
    bool nonResident;
};

// NTFS Boot Sector structure
struct NTFSBootSector {
    uint8_t  jump[3];
    uint8_t  oemID[8];
    uint16_t bytesPerSector;
    uint8_t  sectorsPerCluster;
    uint16_t reservedSectors;
    uint8_t  reserved1[3];
    uint16_t reserved2;
    uint8_t  mediaDescriptor;
    uint16_t reserved3;
    uint16_t sectorsPerTrack;
    uint16_t numberOfHeads;
    uint32_t hiddenSectors;
    uint32_t reserved4;
    uint32_t reserved5;
    uint64_t totalSectors;
    uint64_t mftCluster; // mft start
    uint64_t mirrorMftCluster;
    int8_t   clustersPerMftRecord;
    uint8_t  reserved6[3];
    int8_t   clustersPerIndexBlock;
    uint8_t  reserved7[3];
    uint64_t volumeSerialNumber;
    uint32_t checksum;
};



// Standard MFT entry header
struct MFTEntryHeader {
    uint32_t signature;          // "FILE"
    uint16_t updateSequenceOffset;
    uint16_t updateSequenceSize;
    uint64_t logFileSequenceNumber;
    uint16_t sequenceNumber;
    uint16_t hardLinkCount;
    uint16_t firstAttributeOffset;
    uint16_t flags;             // 0x0001 = InUse, 0x0002 = Directory
    uint32_t usedSize;
    uint32_t allocatedSize;
    uint64_t baseFileRecord;
    uint16_t nextAttributeId;
    uint16_t padding;
    uint32_t recordNumber;
};

// Attribute Header
struct AttributeHeader {
    uint32_t type;              // 0x10 = Standard Information, 0x30 = File Name, etc.
    uint32_t length;
    uint8_t  nonResident;
    uint8_t  nameLength;
    uint16_t nameOffset;
    uint16_t flags;
    uint16_t attributeId;
};

// Resident Attribute Header
struct ResidentAttributeHeader : AttributeHeader {
    uint32_t contentLength;
    uint16_t contentOffset;
};

// Non-Resident Attribute Header
struct NonResidentAttributeHeader : AttributeHeader {
    uint64_t startingVCN;
    uint64_t lastVCN;
    uint16_t dataRunOffset;
    uint16_t compressionUnit;
    uint32_t padding;
    uint64_t allocatedSize;
    uint64_t realSize;
    uint64_t initializedSize;
};

// File Name Attribute
struct FileNameAttribute {
    uint64_t parentDirectory;
    uint64_t creationTime;
    uint64_t modificationTime;
    uint64_t mftModificationTime;
    uint64_t lastAccessTime;
    uint64_t allocatedSize;
    uint64_t realSize;
    uint32_t flags;
    uint32_t reparseValue;
    uint8_t  nameLength;
    uint8_t  nameType;
    wchar_t  name[1];
};

struct NTFSRecoveryStatus {
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