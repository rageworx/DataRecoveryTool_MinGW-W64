#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>


#pragma pack(push, 1)

struct ClusterUsage {
    uint64_t timestamp;  // When this cluster was used
    uint32_t fileId;     // Identifier for the deleted file
    bool isDeleted;      // Whether this usage was from a deleted file
    uint64_t writeOffset; // Offset within the file where this cluster was used
};

struct OverwriteAnalysis {
    bool hasOverwrite;
    std::vector<uint32_t> overwrittenClusters;
    std::map<uint32_t, std::vector<uint32_t>> overwrittenBy; // cluster -> list of file IDs that overwrote it
    double overwritePercentage;
};


#pragma pack(pop)