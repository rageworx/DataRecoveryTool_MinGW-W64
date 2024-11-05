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
    std::wstring drivePath;
    bool openDrive();
public:
    explicit LogicalDriveReader(const std::wstring& drivePath);
    ~LogicalDriveReader() override;

    // Delete copy constructor and assignment to prevent handle duplication
    LogicalDriveReader(const LogicalDriveReader&) = delete;
    LogicalDriveReader& operator=(const LogicalDriveReader&) = delete;

    // Move operations are allowed
    LogicalDriveReader(LogicalDriveReader&& other) noexcept;
    LogicalDriveReader& operator=(LogicalDriveReader&& other) noexcept;

    // Implement SectorReader interface
    bool readSector(uint64_t sector, void* buffer, uint32_t size) override;
    bool getBytesPerSector(uint32_t& bytesPerSector) override;
    std::wstring getFilesystemType() override;
    bool isOpen() const override { return hDrive != INVALID_HANDLE_VALUE; }
    bool reopen() override;
    void close() override;
};