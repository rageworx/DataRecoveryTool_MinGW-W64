#pragma once
//#include "Structures.h"
#include "IConfigurable.h"
#include "SectorReader.h"
#include "Enums.h"
#include <memory>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string_view>





class DriveHandler : public IConfigurable{
private:
    // Constants
    static constexpr int MBR_SIGNATURE_OFFSET = 0x1FE;
    static constexpr int GPT_SIGNATURE_OFFSET = 0x00;
    static constexpr std::wstring_view GPT_SIGNATURE = L"EFI PART";

    // Configuration and state
    
    DriveType driveType;
    FilesystemType fsType;
    PartitionType partitionType;
    uint32_t bytesPerSector{ 0 };
    std::unique_ptr<SectorReader> sectorReader;

    // Filesystem type mapping
    const std::unordered_map<std::wstring, FilesystemType> filesystemMap = {
        {L"exFAT", FilesystemType::EXFAT_TYPE},
        {L"FAT32", FilesystemType::FAT32_TYPE},
        {L"NTFS", FilesystemType::NTFS_TYPE}
    };



    FilesystemType getFilesystemType();
    PartitionType getPartitionType();
    DriveType determineDriveType(const std::wstring& drivePath);


    // Initialize sector reader based on drive type
    void initializeSectorReader();
    // Read data from specified sector
    bool readSector(uint64_t sector, void* buffer, uint32_t size);
    // Set the sector reader implementation 
    void setSectorReader(std::unique_ptr<SectorReader> reader);
    // Get bytes per sector for the drive
    void getBytesPerSector();

    void closeDrive();

    bool isGpt(const uint8_t* buffer);
    bool isMbr(const uint8_t* buffer);

    std::unique_ptr<SectorReader> releaseSectorReader();
public:
    /*=============== Public Interface ===============*/
    // Constructor
    explicit DriveHandler();
    // Destructor
    ~DriveHandler();

    // Prevent copying
    DriveHandler(const DriveHandler&) = delete;
    DriveHandler& operator=(const DriveHandler&) = delete;

    // Allow moving
    DriveHandler(DriveHandler&&) noexcept = default;
    DriveHandler& operator=(DriveHandler&&) noexcept = default;

    // Main recovery entry point
    void recoverDrive();


};