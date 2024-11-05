//#include "PhysicalDriveReader.h"
//
//
///*=============== Constructor & Destructor ===============*/
//// Initialize reader with drive path
//PhysicalDriveReader::PhysicalDriveReader(const std::wstring& drivePath, uint64_t startSector) : partitionOffset(startSector) {
//    openDrive(drivePath);
//}
//// Ensure drive handle is closed
//PhysicalDriveReader::~PhysicalDriveReader() {
//    closeDrive();
//}
//
//
///*=============== Drive Access Methods ===============*/
//// Read data from specified sector (implements SectorReader interface)
//bool PhysicalDriveReader::getBytesPerSector(uint32_t& bytesPerSector) {
//
//    DISK_GEOMETRY dg;
//    DWORD bytesReturned;
//
//    // Retrieve drive geometry using DeviceIoControl
//    if (DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &dg, sizeof(dg), &bytesReturned, NULL)) {
//        bytesPerSector = dg.BytesPerSector;
//        return true;
//    }
//    else {
//        bytesPerSector = 0;
//        return false;
//    }
//
//}
//
//
//std::wstring PhysicalDriveReader::getFilesystemType() {
//    wchar_t fileSystemNameBuffer[MAX_PATH];
//
//    if (GetVolumeInformationByHandleW(
//        hDrive,       
//        NULL,                    // Volume name (not needed here)
//        0,                       // Size of volume name buffer
//        NULL,                    // Volume serial number
//        NULL,                    // Maximum component length
//        NULL,                    // File system flags
//        fileSystemNameBuffer,    // File system name buffer
//        MAX_PATH))               // Buffer size
//    {
//        std::wcout << L"Filesystem type is: " << fileSystemNameBuffer << std::endl;
//        return std::wstring(fileSystemNameBuffer);
//    }
//    else {
//        std::wcerr << L"Failed to retrieve filesystem type. Error: " << GetLastError() << std::endl;
//        return L"";
//    }
//}
//
//// Get drive's bytes per sector (implements SectorReader interface)
//bool PhysicalDriveReader::readSector(uint64_t sector, void* buffer, uint32_t size) {
//    LARGE_INTEGER offset;
//    offset.QuadPart = (sector + partitionOffset) * size;
//    DWORD bytesRead;
//
//    if (!SetFilePointerEx(hDrive, offset, NULL, FILE_BEGIN)) {
//        std::cerr << "Failed to set file pointer to sector offset. (PhysicalDriveReader::readSector)" << std::endl;
//        return false;
//    }
//
//    if (!ReadFile(hDrive, buffer, size, &bytesRead, NULL)) {
//        std::cerr << "Failed to read from drive. (PhysicalDriveReader::readSector)" << std::endl;
//        return false;
//    }
//
//    if (bytesRead != size) {
//        std::cerr << "Incomplete sector read. (PhysicalDriveReader::readSector)" << std::endl;
//        return false;
//    }
//    return true;
//
//}
///*=============== Drive Handle Management ===============*/
//// Open drive for reading
//void PhysicalDriveReader::openDrive(const std::wstring& drivePath) {
//    hDrive = CreateFileW(
//        drivePath.c_str(),
//        GENERIC_READ,
//        FILE_SHARE_READ,
//        NULL,
//        OPEN_EXISTING,
//        0,
//        NULL
//    );
//    if (hDrive == INVALID_HANDLE_VALUE) {
//        DWORD lastError = GetLastError();
//        if (lastError == 5) {
//            throw std::runtime_error("You have to run this program as Administrator when opening Physical drive.");
//        }
//        throw std::runtime_error("Failed to open physical drive. Please make sure to enter the correct drive.");
//    }
//}
//// Close drive handle
//void PhysicalDriveReader::closeDrive() {
//    if (hDrive != INVALID_HANDLE_VALUE) {
//        CloseHandle(hDrive);
//    }
//}
//
