#pragma once
#include "SectorReader.h"
#include <cstdint>
#include <string>
#include <windows.h>
#include <algorithm>
#include <fstream>
#include <iostream>

class LogicalDriveReader : public SectorReader {
private:
    HANDLE hDrive;
public:
    /*=============== Constructor & Destructor ===============*/
    // Initialize reader with drive path
    explicit LogicalDriveReader(const std::wstring& drivePath);
    // Ensure drive handle is closed
    ~LogicalDriveReader();

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