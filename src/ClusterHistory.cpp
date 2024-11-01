#include "ClusterHistory.h"


void ClusterHistory::recordClusterUsage(uint32_t cluster, uint32_t fileId, uint64_t writeOffset) {
    ClusterUsage usage{
        .timestamp = static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count()),
        .fileId = fileId,
        .isDeleted = true,
        .writeOffset = writeOffset
    };
    clusterUsageHistory[cluster].push_back(usage);
}

std::vector<std::pair<ClusterUsage, ClusterUsage>> ClusterHistory::findOverlappingUsage(uint32_t cluster) {
    std::vector<std::pair<ClusterUsage, ClusterUsage>> overlaps;
    auto& history = clusterUsageHistory[cluster];

    for (size_t i = 0; i < history.size(); i++) {
        for (size_t j = i + 1; j < history.size(); j++) {
            if (history[i].fileId != history[j].fileId &&
                history[i].isDeleted && history[j].isDeleted) {
                overlaps.push_back({ history[i], history[j] });
            }
        }
    }
    return overlaps;
}
