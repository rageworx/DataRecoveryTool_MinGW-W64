#pragma once
#include "Structures.h"
#include <cstdint>
#include <map>
#include <vector>
#include <chrono>

class ClusterHistory {
public:


    std::map<uint32_t, std::vector<ClusterUsage>> clusterUsageHistory;

    void recordClusterUsage(uint32_t cluster, uint32_t fileId, uint64_t writeOffset);

    std::vector<std::pair<ClusterUsage, ClusterUsage>> findOverlappingUsage(uint32_t cluster);
};