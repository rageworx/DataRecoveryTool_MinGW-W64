#include "SectorReader.h"
#include "exFATRecovery.h"
#include <cstdint>
#include <memory>
#include <vector>
#include <fstream>
//#include <iostream>


// Helper functions for entry type checking
inline bool exFATRecovery::IsDirectoryEntry(uint8_t entryType) {
    return (entryType & 0x7F) == 0x05;  // 0x85 with status bit masked
}

inline bool exFATRecovery::IsStreamExtensionEntry(uint8_t entryType) {
    return (entryType & 0x7F) == 0x40;  // 0xC0 with status bit masked
}

inline bool exFATRecovery::IsFileNameEntry(uint8_t entryType) {
    return (entryType & 0x7F) == 0x41;  // 0xC1 with status bit masked
}

inline bool exFATRecovery::IsEntryInUse(uint8_t entryType) {
    return (entryType & 0x80) != 0;  // Check if in-use bit is set
}

void exFATRecovery::initializeSectorReader() {
    auto driveReader = std::make_unique<LogicalDriveReader>(config.drivePath);
    setSectorReader(std::move(driveReader));

    if (sectorReader == nullptr) {
        throw std::runtime_error("Failed to initialize Sector Reader.");
    }
}

void exFATRecovery::readBootSector(uint32_t sector) {
    const uint32_t sectorSize = sizeof(ExFATBootSector);
    ExFATBootSector bootSector;

    if (!readSector(sector, &bootSector, sectorSize)) {
        throw std::runtime_error("Failed to read exFAT boot sector");
    }

    if (memcmp(bootSector.FileSystemName, "EXFAT   ", 8) != 0) {
        throw std::runtime_error("Not a valid exFAT volume");
    }

    this->bytesPerSector = 1 << bootSector.BytesPerSectorShift;
    this->sectorsPerCluster = 1 << bootSector.SectorsPerClusterShift;
    this->fatOffset = bootSector.FatOffset;
    this->clusterHeapOffset = bootSector.ClusterHeapOffset;
    this->rootDirectoryCluster = bootSector.RootDirectoryCluster;
    this->clusterCount = bootSector.ClusterCount;

    // Resize sector buffer if needed
    if (this->bytesPerSector > sectorBuffer.size()) {
        sectorBuffer.resize(this->bytesPerSector);
    }

    this->volumeFlags = bootSector.VolumeFlags;
    this->numberOfFats = bootSector.NumberOfFats;
    this->volumeLength = bootSector.VolumeLength;
}

// Read data from specified sector
bool exFATRecovery::readSector(uint32_t sector, void* buffer, uint32_t size) {
    return sectorReader->readSector(sector, buffer, size);
}

exFATRecovery::exFATRecovery(const Config& cfg) : config(cfg) {
    initializeSectorReader();
    readBootSector(0);
}

// Set the sector reader implementation
void exFATRecovery::setSectorReader(std::unique_ptr<SectorReader> reader) {
    sectorReader = std::move(reader);
}

exFATRecovery::~exFATRecovery() {

}
