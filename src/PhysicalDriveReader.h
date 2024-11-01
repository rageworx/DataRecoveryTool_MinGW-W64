#pragma once
#include "SectorReader.h"
#include <cstdint>
#include <string>
#include <windows.h>
#include <algorithm>
#include <fstream>
#include <iostream>

class PhysicalDriveReader : public SectorReader {
private:
    HANDLE hDrive;
    uint64_t partitionOffset;

public:
    /*=============== Constructor & Destructor ===============*/
    // Initialize reader with drive path
    PhysicalDriveReader(const std::wstring& drive, uint64_t startSector);
    // Ensure drive handle is closed
    ~PhysicalDriveReader();

    /*=============== Drive Access Methods ===============*/
    // Read data from specified sector (implements SectorReader interface)
    bool readSector(uint64_t sector, void* buffer, uint32_t size) override;
    // Get drive's bytes per sector (implements SectorReader interface)
    bool getBytesPerSector(uint32_t& bytesPerSector) override;

    /*=============== Drive Handle Management ===============*/
    // Open drive for reading
    void openDrive(const std::wstring& drivePath);
    // Close drive handle
    void closeDrive();


};
