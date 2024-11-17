#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>


#pragma pack(push, 1)

/* ========= Cluster analysis =========*/
struct ClusterAnalysisResult {
    double fragmentation;      // 0.0-1.0, higher means more fragmented
    bool isCorrupted;
    uint32_t backJumps;
    uint32_t repeatedClusters;
    uint32_t largeGaps;
};

struct RecoveryStatus {
    bool isCorrupted;
    bool hasFragmentedClusters;
    double fragmentation;
    bool hasBackJumps;
    uint32_t backJumps;
    bool hasRepeatedClusters;
    uint32_t repeatedClusters;
    bool hasLargeGaps;
    uint32_t largeGaps;
    bool hasOverwrittenClusters;
    bool hasInvalidFileName;
    bool hasInvalidExtension;
    uint64_t expectedClusters;
    uint64_t recoveredClusters;
    uint64_t recoveredBytes;
    std::vector<uint64_t> problematicClusters;
};

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