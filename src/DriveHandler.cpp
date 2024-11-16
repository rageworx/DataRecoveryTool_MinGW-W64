#include "DriveHandler.h"
#include "FAT32Recovery.h"
#include "exFATRecovery.h"
#include "NTFSRecovery.h"
//#include "PhysicalDriveReader.h"
#include "LogicalDriveReader.h"
#include <cwctype>
#include <iostream>
#include <algorithm>

/*=============== Private Interface ===============*/

/*=============== Filesystem type identification ===============*/


FilesystemType DriveHandler::getFilesystemType() {

    std::wstring fsType = this->sectorReader->getFilesystemType();
    if (fsType == L"UNKNOWN") return FilesystemType::UNKNOWN_TYPE;
    else if (fsType == L"exFAT") return FilesystemType::EXFAT_TYPE;
    else if (fsType == L"FAT32") return FilesystemType::FAT32_TYPE;
    else if (fsType == L"NTFS") return FilesystemType::NTFS_TYPE;

    return FilesystemType::UNKNOWN_TYPE;
}

/*=============== Partition type identification ===============*/
bool DriveHandler::isGpt(const uint8_t* buffer) {
    return std::memcmp(buffer + GPT_SIGNATURE_OFFSET, "EFI PART", 8) == 0;
}
bool DriveHandler::isMbr(const uint8_t* buffer) {
    return buffer[MBR_SIGNATURE_OFFSET] == 0x55 && buffer[MBR_SIGNATURE_OFFSET + 1] == 0xAA;
}

PartitionType DriveHandler::getPartitionType() {
    uint8_t* buffer = new uint8_t[this->bytesPerSector];
    readSector(0, buffer, this->bytesPerSector);
    if (isMbr(buffer)) { // Same signature is present in both GPT and MBR at the same offset
        readSector(1, buffer, this->bytesPerSector);
        if (isGpt(buffer)) return PartitionType::GPT_TYPE;
        return PartitionType::MBR_TYPE;
    }
    return PartitionType::UNKNOWN_TYPE;
}



/*=============== Core Drive Operations ===============*/
// Initialize sector reader based on drive type
void DriveHandler::initializeSectorReader() {
    // It is okay to use LogicalDriveReader, as they only differ in partition offset value, which is not important when reading the first few sectors.
    if (this->driveType == DriveType::LOGICAL_TYPE) {
        auto driveReader = std::make_unique<LogicalDriveReader>(this->config.drivePath);
        setSectorReader(std::move(driveReader));
    }
    else if (this->driveType == DriveType::PHYSICAL_TYPE) {
        throw std::runtime_error("Physical drive recovery is not implemented yet.");
       // auto driveReader = std::make_unique<PhysicalDriveReader>(this->config.drivePath, 0);
        //setSectorReader(std::move(driveReader));
    }
    else {
        throw std::runtime_error("Could not determine the type of the drive (Physical, Logical). Please make sure to enter the correct drive.");
    }


}

// Read data from specified sector
void DriveHandler::readSector(uint64_t sector, void* buffer, uint32_t size) {
    if (!sectorReader->readSector(sector, buffer, size)) {
        throw std::runtime_error("Cannot read sector.");
    }
}
// Set the sector reader implementation
void DriveHandler::setSectorReader(std::unique_ptr<SectorReader> reader) {
    sectorReader = std::move(reader);
    if (sectorReader == nullptr) {
        throw std::runtime_error("Failed to initialize Sector Reader.");
    }
}
// Get bytes per sector for the drive
void DriveHandler::getBytesPerSector() {
    this->bytesPerSector = sectorReader->getBytesPerSector();
    if (this->bytesPerSector == 0) {
        throw std::runtime_error("Failed to read BytesPerSector value using DeviceIoControl");
    }
}
// Close drive handle before recovery
void DriveHandler::closeDrive() {
    if (sectorReader) {
        sectorReader.reset();
    }
}

/*=============== Drive and Partition Analysis ===============*/
// Determine if drive is logical or physical
DriveType DriveHandler::determineDriveType(const std::wstring& drivePath) {
    std::wstring upperDrivePath = drivePath;
    std::transform(upperDrivePath.begin(), upperDrivePath.end(), upperDrivePath.begin(), std::towupper);

    std::wstring path = L"\\\\.\\";

    // Open Physical Drive with only a number
    if (drivePath.size() == 1 && std::isdigit(drivePath[0])) {
        config.drivePath = path + L"PhysicalDrive" + drivePath;
        std::cerr << "Raw disk access is not implemented." << std::endl;
        exit(0);
        //return DriveType::PHYSICAL_TYPE;
    }
    // Case-insensitive, find PhysicalDrive and a number in the input
    else if (upperDrivePath.find(L"PHYSICALDRIVE") != std::wstring::npos &&
        std::isdigit(upperDrivePath[upperDrivePath.size() - 1])) {
        std::cerr << "Raw disk access is not implemented." << std::endl;
        exit(0);
        //config.drivePath = path + L"PhysicalDrive" + upperDrivePath[upperDrivePath.size() - 1];
        //return DriveType::PHYSICAL_TYPE;
    }
    // Case-insensitive, colon not necessary
    else if ((upperDrivePath.size() == 1 && std::iswalpha(upperDrivePath[0])) ||
        (upperDrivePath.size() == 2 && std::iswalpha(upperDrivePath[0]) && upperDrivePath[1] == L':')) {
        config.drivePath = path + upperDrivePath + (upperDrivePath.size() == 1 ? L":" : L"");
        return DriveType::LOGICAL_TYPE;
    }

    return DriveType::UNKNOWN_TYPE;
}

/*=============== Public Interface ===============*/
// Constructor
DriveHandler::DriveHandler(const Config& cfg)
    : config(cfg)
{
    this->driveType = determineDriveType(this->config.drivePath);

    initializeSectorReader();
    getBytesPerSector();

    this->fsType = getFilesystemType();
    this->partitionType = getPartitionType();
}
// Main recovery entry point
void DriveHandler::recoverDrive() {
    
    if (this->driveType == DriveType::UNKNOWN_TYPE) {
        throw std::runtime_error("Drive type unknown");
    }

    if (this->fsType == FilesystemType::UNKNOWN_TYPE) {
        throw std::runtime_error("Filesystem type unknown");
    }

    if (this->partitionType == PartitionType::UNKNOWN_TYPE) {
        throw std::runtime_error("Partition type unknown");
    }

    
    if (this->fsType == FilesystemType::FAT32_TYPE) {
        FAT32Recovery recovery(this->config, this->driveType, this->partitionType);
        recovery.startRecovery(releaseSectorReader());
    }
    else if (this->fsType == FilesystemType::EXFAT_TYPE) {
        exFATRecovery recovery(this->config, this->driveType, releaseSectorReader());
        recovery.startRecovery();
    }
    else if (this->fsType == FilesystemType::NTFS_TYPE) {
        NTFSRecovery recovery(this->config, this->driveType, releaseSectorReader());
        recovery.startRecovery();
    }
}


