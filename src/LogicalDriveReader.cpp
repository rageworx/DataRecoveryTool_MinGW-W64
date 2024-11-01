#pragma once
#include "LogicalDriveReader.h"
#include "SectorReader.h"



/*=============== Constructor & Destructor ===============*/
// Initialize reader with drive path
LogicalDriveReader::LogicalDriveReader(const std::wstring& drivePath) {
    openDrive(drivePath);

}
// Ensure drive handle is closed
LogicalDriveReader::~LogicalDriveReader() {
    //if (hDrive != INVALID_HANDLE_VALUE) {
    closeDrive();
    //}
}


/*=============== Drive Access Methods ===============*/
// Read data from specified sector (implements SectorReader interface)
bool LogicalDriveReader::readSector(uint64_t sector, void* buffer, uint32_t size){
    LARGE_INTEGER offset;
    offset.QuadPart = sector * size;
    DWORD bytesRead;

    if (!SetFilePointerEx(hDrive, offset, NULL, FILE_BEGIN)) {
        std::cerr << "Failed to set file pointer to sector offset. (DriveLetterReader::readSector)" << std::endl;
        return false;
    }

    if (!ReadFile(hDrive, buffer, size, &bytesRead, NULL)) {
        std::cerr << "Failed to read from drive. (DriveLetterReader::readSector)" << std::endl;
        return false;
    }

    if (bytesRead != size) {
        std::cerr << "Incomplete sector read. (DriveLetterReader::readSector)" << std::endl;
        return false;
    }
    return true;
}
// Get drive's bytes per sector (implements SectorReader interface)
bool LogicalDriveReader::getBytesPerSector(uint32_t& bytesPerSector) {
    
    DISK_GEOMETRY dg;
    DWORD bytesReturned;

    // Retrieve drive geometry using DeviceIoControl
    if (DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &dg, sizeof(dg), &bytesReturned, NULL)) {
        bytesPerSector = dg.BytesPerSector;
        return true;
    }
    else {
        bytesPerSector = 0;
        return false;
    }
    
}


/*=============== Drive Handle Management ===============*/
// Open drive for reading
void LogicalDriveReader::openDrive(const std::wstring& drivePath) {
   

    hDrive = CreateFileW(
        drivePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    if (hDrive == INVALID_HANDLE_VALUE) {
        DWORD lastError = GetLastError();
        if (lastError == 5) {
            throw std::runtime_error("You have to run this program as Administrator.");
        }
        throw std::runtime_error("Failed to open logical drive. Please make sure to enter the correct drive.");
    }
}
// Close drive handle
void LogicalDriveReader::closeDrive() {
    if (hDrive != INVALID_HANDLE_VALUE) {
        CloseHandle(hDrive);
        //hDrive = INVALID_HANDLE_VALUE;
    }
}

