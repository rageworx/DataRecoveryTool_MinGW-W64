#include "DriveHandler.h"
#include "FAT32Recovery.h"
#include "exFATRecovery.h"
#include "NTFSRecovery.h"

#include "LogicalDriveReader.h"
#include <cwctype>
#include <iostream>
#include <algorithm>


// Constructor
DriveHandler::DriveHandler() : IConfigurable(){
    try {
        driveType = determineDriveType(config.drivePath);
        if (driveType == DriveType::UNKNOWN_TYPE) {
            throw std::runtime_error("Unknown drive type");
        }

        if (driveType == DriveType::PHYSICAL_TYPE) {
            throw std::runtime_error("Physical drive recovery not implemented");
        }

        initializeSectorReader();
        getBytesPerSector();

        fsType = getFilesystemType();
        //partitionType = getPartitionType();
    }
    catch (const std::exception& e) {
        std::cerr << "DriveHandler Constructor exception: " << e.what() << std::endl;
        sectorReader.reset();
        throw;
    }
}
// Destructor
DriveHandler::~DriveHandler() {
    closeDrive();
}

// Determine if drive is logical or physical
DriveType DriveHandler::determineDriveType(const std::wstring& drivePath) {
    std::wstring upperPath = drivePath;
    std::wstring path = L"\\\\.\\";

    std::transform(upperPath.begin(), upperPath.end(),
        upperPath.begin(), std::towupper);

    // Single digit drive number
    if (drivePath.size() == 1 && std::isdigit(drivePath[0])) {
        //config.drivePath = path + L"PhysicalDrive" + drivePath;
        return DriveType::PHYSICAL_TYPE;
    }

    // Physical drive with explicit prefix
    if (upperPath.find(L"PHYSICALDRIVE") != std::wstring::npos &&
        std::isdigit(upperPath.back())) {
        //config.drivePath = path + L"PhysicalDrive" + upperPath[upperPath.size() - 1];
        return DriveType::PHYSICAL_TYPE;
    }

    // Logical drive letter
    if ((upperPath.size() == 1 && std::iswalpha(upperPath[0])) ||
        (upperPath.size() == 2 && std::iswalpha(upperPath[0]) &&
            upperPath[1] == L':')) {
        config.drivePath = path + upperPath + (upperPath.size() == 1 ? L":" : L"");
        return DriveType::LOGICAL_TYPE;
    }

    return DriveType::UNKNOWN_TYPE;
}
// FAT32, NTFS, ...
FilesystemType DriveHandler::getFilesystemType() {
    std::wstring fsType = sectorReader->getFilesystemType();
    return filesystemMap.find(fsType) != filesystemMap.end()
        ? filesystemMap.at(fsType)
        : FilesystemType::UNKNOWN_TYPE;
}
// MBR, GPT - for physical drive, not implemented
PartitionType DriveHandler::getPartitionType() {
    std::vector<uint8_t> buffer(bytesPerSector);

    if (!readSector(0, buffer.data(), bytesPerSector)) {
        throw std::runtime_error("Failed to read partition table");
    }

    if (isMbr(buffer.data())) {
        if (!readSector(1, buffer.data(), bytesPerSector)) {
            throw std::runtime_error("Failed to read potential GPT header");
        }
        return isGpt(buffer.data()) ? PartitionType::GPT_TYPE
            : PartitionType::MBR_TYPE;
    }
    return PartitionType::UNKNOWN_TYPE;
}

// Initialize sector reader based on drive type
void DriveHandler::initializeSectorReader() {
    switch (driveType) {
    case DriveType::LOGICAL_TYPE:
        setSectorReader(std::make_unique<LogicalDriveReader>(config.drivePath));
        break;
    case DriveType::PHYSICAL_TYPE:
        throw std::runtime_error("Physical drive recovery not implemented");
    default:
        throw std::runtime_error("Invalid drive type");
    }
}
// Read data from specified sector
bool DriveHandler::readSector(uint64_t sector, void* buffer, uint32_t size) {
    return sectorReader && sectorReader->readSector(sector, buffer, size);
}
// Set the sector reader implementation
void DriveHandler::setSectorReader(std::unique_ptr<SectorReader> reader) {
    if (!reader) {
        throw std::runtime_error("Invalid sector reader");
    }
    sectorReader = std::move(reader);
}
// Get bytes per sector for the drive
void DriveHandler::getBytesPerSector() {
    if (!sectorReader) {
        throw std::runtime_error("Sector reader not initialized");
    }

    bytesPerSector = sectorReader->getBytesPerSector();
    if (bytesPerSector == 0) {
        throw std::runtime_error("Invalid bytes per sector");
    }
}


bool DriveHandler::isGpt(const uint8_t* buffer) {
    return std::equal(
        GPT_SIGNATURE.begin(),
        GPT_SIGNATURE.end(),
        reinterpret_cast<const wchar_t*>(buffer + GPT_SIGNATURE_OFFSET)
    );
}
bool DriveHandler::isMbr(const uint8_t* buffer) {
    return buffer[MBR_SIGNATURE_OFFSET] == 0x55 &&
        buffer[MBR_SIGNATURE_OFFSET + 1] == 0xAA;
}

std::unique_ptr<SectorReader> DriveHandler::releaseSectorReader() {
    return std::move(sectorReader);
}

void DriveHandler::closeDrive() {
    if (sectorReader) {
        sectorReader.reset();
    }
}
/*=============== Public Interface ===============*/

// Main recovery entry point
void DriveHandler::recoverDrive() {
    if (!sectorReader) {
        throw std::runtime_error("Drive not initialized");
    }

    // Create appropriate recovery handler based on filesystem type
    switch (fsType) {
    case FilesystemType::FAT32_TYPE:
        FAT32Recovery(driveType, releaseSectorReader())
            .startRecovery();
        break;
    case FilesystemType::EXFAT_TYPE:
        exFATRecovery(driveType, releaseSectorReader())
            .startRecovery();
        break;
    case FilesystemType::NTFS_TYPE:
        NTFSRecovery(driveType, releaseSectorReader())
            .startRecovery();
        break;
    default:
        throw std::runtime_error("Unsupported filesystem type");
    }
}


