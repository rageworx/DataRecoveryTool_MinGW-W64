#pragma once
#include "LogicalDriveReader.h"
#include "SectorReader.h"


LogicalDriveReader::LogicalDriveReader(const std::wstring& path)
    : hDrive(INVALID_HANDLE_VALUE)
    , drivePath(path) {
    if (!openDrive()) {
        throw std::runtime_error("Failed to initialize drive reader");
    }
}

LogicalDriveReader::~LogicalDriveReader() {
    close();
}

LogicalDriveReader::LogicalDriveReader(LogicalDriveReader&& other) noexcept
    : hDrive(other.hDrive)
    , drivePath(std::move(other.drivePath)) {
    other.hDrive = INVALID_HANDLE_VALUE;
}

LogicalDriveReader& LogicalDriveReader::operator=(LogicalDriveReader&& other) noexcept {
    if (this != &other) {
        close();
        hDrive = other.hDrive;
        drivePath = std::move(other.drivePath);
        other.hDrive = INVALID_HANDLE_VALUE;
    }
    return *this;
}

bool LogicalDriveReader::openDrive() {
    close(); // Ensure any existing handle is closed

    hDrive = CreateFileW(
        drivePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, // Optimize for random access
        NULL
    );

    if (hDrive == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_ACCESS_DENIED) {
            throw std::runtime_error("Administrator privileges required");
        }
        return false;
    }
    return true;
}

bool LogicalDriveReader::reopen() {
    return openDrive();
}

void LogicalDriveReader::close() {
    if (hDrive != INVALID_HANDLE_VALUE) {
        CloseHandle(hDrive);
        hDrive = INVALID_HANDLE_VALUE;
    }
}

bool LogicalDriveReader::readSector(uint64_t sector, void* buffer, uint32_t size) {
    if (!isOpen()) {
        if (!reopen()) {
            return false;
        }
    }

    LARGE_INTEGER offset;
    offset.QuadPart = sector * size;
    DWORD bytesRead;

    // Set file pointer to sector offset
    if (!SetFilePointerEx(hDrive, offset, NULL, FILE_BEGIN)) {
        return false;
    }

    // Read the sector
    if (!ReadFile(hDrive, buffer, size, &bytesRead, NULL) || bytesRead != size) {
        return false;
    }

    return true;
}

bool LogicalDriveReader::getBytesPerSector(uint32_t& bytesPerSector) {
    if (!isOpen()) {
        if (!reopen()) {
            return false;
        }
    }

    DISK_GEOMETRY dg;
    DWORD bytesReturned;

    if (!DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY,
        NULL, 0, &dg, sizeof(dg), &bytesReturned, NULL)) {
        bytesPerSector = 0;
        return false;
    }

    bytesPerSector = dg.BytesPerSector;
    return true;
}

std::wstring LogicalDriveReader::getFilesystemType() {
    if (!isOpen()) {
        if (!reopen()) {
            return L"UNKNOWN_TYPE";
        }
    }

    wchar_t fileSystemName[MAX_PATH];
    if (!GetVolumeInformationByHandleW(
        hDrive,
        NULL,
        0,
        NULL,
        NULL,
        NULL,
        fileSystemName,
        MAX_PATH)) {
        return L"UNKNOWN_TYPE";
    }

    return fileSystemName;
}