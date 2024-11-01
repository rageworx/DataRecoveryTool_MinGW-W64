#pragma once
#include <cstdint>
/*=============== Abstract Sector Reader Interface ===============*/
class SectorReader {
public:
    // Read specified number of bytes from given sector
    virtual bool readSector(uint64_t sector, void* buffer, uint32_t size) = 0;

    // Get drive's bytes per sector
    virtual bool getBytesPerSector(uint32_t& bytesPerSector) = 0;

    // Virtual destructor for cleanup
    virtual ~SectorReader() = default;
};