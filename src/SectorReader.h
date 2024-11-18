#pragma once
#include <cstdint>
#include <string>

class SectorReader {
public:
    virtual bool readSector(uint64_t sector, void* buffer, uint32_t size) = 0;
    virtual uint32_t getBytesPerSector() = 0;
    virtual std::wstring getFilesystemType() = 0;
    virtual uint64_t getTotalMftRecords() = 0;
    virtual bool isOpen() const = 0;
    virtual bool reopen() = 0;
    virtual void close() = 0;
    virtual ~SectorReader() = default;
};