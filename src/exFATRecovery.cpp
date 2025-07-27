#include "SectorReader.h"
#include "exFATRecovery.h"
#include <cstdint>
#include <memory>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <set>

exFATRecovery::exFATRecovery(const DriveType& driveType, std::unique_ptr<SectorReader> reader)
    : IConfigurable(), driveType(driveType) {
    printToolHeader();

    utils.ensureOutputDirectory();
    setSectorReader(std::move(reader));
    readBootSector(0);
}

exFATRecovery::~exFATRecovery() {
    utils.closeLogFile();
}

void exFATRecovery::printToolHeader() const {
    std::cout << "\n";
    std::cout << "\n";
    std::cout << " ************************************************************************\n";
    std::cout << " *            _____ _  _____   ____                                     *\n";
    std::cout << " *   _____  _|  ___/ \\|_   _| |  _ \\ ___  ___ _____   _____ _ __ _   _  *\n";
    std::cout << " *  / _ \\ \\/ / |_ / _ \\ | |   | |_) / _ \\/ __/ _ \\ \\ / / _ \\ '__| | | | *\n";
    std::cout << " * |  __/>  <|  _/ ___ \\| |   |  _ <  __/ (_| (_) \\ V /  __/ |  | |_| | *\n";
    std::cout << " *  \\___/_/\\_\\_|/_/   \\_\\_|   |_| \\_\\___|\\___\\___/ \\_/ \\___|_|   \\__, | *\n";
    std::cout << " *                                                               |___/  *\n";
    std::cout << " ************************************************************************\n";
    std::cout << "\n";
    std::cout << "\n";
}

// Helper functions for entry type checking
inline bool exFATRecovery::IsDirectoryEntry(uint8_t entryType) {
    return (entryType & 0x7F) == 0x05;  // 0x85 with status bit masked
}

inline bool exFATRecovery::IsStreamExtensionEntry(uint8_t entryType) {
    return (entryType & 0x7F) == 0x40;  // 0xC0 with status bit masked
}

inline bool exFATRecovery::IsFileNameEntry(uint8_t entryType) {
    return (entryType & 0x7F) == 0x41;  // 0xC1 with status bit masked
}

inline bool exFATRecovery::IsEntryInUse(uint8_t entryType) {
    return (entryType & 0x80) != 0;  // Check if in-use bit is set
}

/* Sector and drive operations */
void exFATRecovery::setSectorReader(std::unique_ptr<SectorReader> reader) {
    if (!reader) {
        throw std::runtime_error("Invalid sector reader");
    }
    sectorReader = std::move(reader);
}

bool exFATRecovery::readSector(uint64_t sector, void* buffer, uint32_t size) {
    return sectorReader && sectorReader->readSector(sector, buffer, size);
}

void exFATRecovery::readBootSector(uint32_t sector) {
    uint32_t bytesPerSector = getBytesPerSector();
    std::vector<uint8_t> buffer(bytesPerSector);

    if (!readSector(sector, buffer.data(), bytesPerSector)) {
        throw std::runtime_error("Failed to read exFAT boot sector");
    }

    driveInfo.bootSector = *reinterpret_cast<ExFATBootSector*>(buffer.data());

    if (memcmp(driveInfo.bootSector.FileSystemName, "EXFAT   ", 8) != 0) {
        throw std::runtime_error("Not a valid exFAT volume");
    }

    driveInfo.bytesPerSector = 1 << driveInfo.bootSector.BytesPerSectorShift;
    driveInfo.sectorsPerCluster = 1 << driveInfo.bootSector.SectorsPerClusterShift;

    /*driveInfo.fatOffset = driveInfo.bootSector.FatOffset;
    driveInfo.clusterHeapOffset = driveInfo.bootSector.ClusterHeapOffset;
    driveInfo.rootDirectoryCluster = driveInfo.bootSector.RootDirectoryCluster;
    driveInfo.clusterCount = driveInfo.bootSector.ClusterCount;*/

 /*   if (driveInfo.bytesPerSector > sectorBuffer.size()) {
        sectorBuffer.resize(driveInfo.bytesPerSector);
    }*/

    //driveInfo.volumeFlags = driveInfo.bootSector.VolumeFlags;
    //driveInfo.numberOfFats = driveInfo.bootSector.NumberOfFats;
    //driveInfo.volumeLength = driveInfo.bootSector.VolumeLength;
}

uint32_t exFATRecovery::getBytesPerSector() {
    if (!sectorReader) {
        throw std::runtime_error("Sector reader not initialized");
    }

    uint32_t bytesPerSector = sectorReader->getBytesPerSector();
    if (bytesPerSector == 0) {
        throw std::runtime_error("Invalid bytes per sector");
    }
    return bytesPerSector;
}

/* Validation */
bool exFATRecovery::isValidDeletedEntry(uint32_t cluster, uint64_t size) const {
    if (size == 0) return false;

    uint64_t volumeSize = driveInfo.bootSector.VolumeLength * driveInfo.bytesPerSector;
    if (size > volumeSize) return false;

    return isValidCluster(cluster);
}
bool exFATRecovery::isValidCluster(uint32_t cluster) const {
    // Basic range check
    if (cluster < MIN_DATA_CLUSTER || cluster > driveInfo.bootSector.ClusterCount) {
        return false;
    }

    // Check if cluster is in valid range but marked as bad or EOF
    if (cluster >= BAD_CLUSTER) {
        return false;
    }

    return true;
}

/* Cluster operations*/
uint32_t exFATRecovery::clusterToSector(uint32_t cluster) {
    // Convert the cluster number to a sector number using the Cluster Heap Offset
    return driveInfo.bootSector.ClusterHeapOffset + ((cluster - 2) * driveInfo.sectorsPerCluster);
}
uint32_t exFATRecovery::getNextCluster(uint32_t cluster) {
    uint32_t fatOffset = cluster * 4;
    uint32_t fatSector = driveInfo.bootSector.ClusterHeapOffset + (fatOffset / driveInfo.bytesPerSector);
    uint32_t entryOffset = fatOffset % driveInfo.bytesPerSector;

    std::vector<uint8_t> sectorBuffer(driveInfo.bytesPerSector);
    if (!readSector(fatSector, sectorBuffer.data(), driveInfo.bytesPerSector)) {
        std::cerr << "Error: Failed to read FAT sector " << fatSector << std::endl;
        return 0xFFFFFFFF;
    }
    
    uint32_t fatEntry = *reinterpret_cast<uint32_t*>(sectorBuffer.data() + entryOffset);
    uint32_t nextCluster = fatEntry & 0x0FFFFFFF;  // Mask the lower 28 bits

        // Check for special cluster values
    if (nextCluster >= 0x0FFFFFF8) {
        if (nextCluster == 0x0FFFFFFF) {
            return 0xFFFFFFFF;  // End of cluster chain
        }
        else {
            return 0xFFFFFFF7;  // Bad cluster
        }
    }

    return nextCluster;
}

/* File scan */
void exFATRecovery::scanForDeletedFiles() {
    utils.printHeader("File Search:");
    if (!utils.openLogFile() && !utils.confirmProceedWithoutLogFile()) {
        std::cout << "Exitting..." << std::endl;
        exit(1);
    }
    scanDirectory(driveInfo.bootSector.RootDirectoryCluster);
    utils.closeLogFile();
    utils.printFooter();
}

void exFATRecovery::scanDirectory(uint32_t cluster, uint32_t depth) {
    try {
        
        if (depth >= MAX_RECURSION_DEPTH) {
            throw std::runtime_error("[-] Maximum directory depth exceeded");
        }

        if (!isValidCluster(cluster)) {
            std::cerr << "[!] Invalid cluster detected: 0x"
                << std::hex << cluster << std::dec << std::endl;
            return;
        }

        uint64_t sector = clusterToSector(cluster);
        uint32_t entriesPerSector = driveInfo.bytesPerSector / sizeof(DirectoryEntryCommon);
        uint64_t maxSectorCount = static_cast<uint64_t>(driveInfo.bootSector.ClusterCount) * static_cast<uint64_t>(driveInfo.sectorsPerCluster);
        //sectorBuffer.resize(bytesPerSector);
        std::vector<uint8_t> sectorBuffer(driveInfo.bytesPerSector);
        for (uint32_t i = 0; i < driveInfo.sectorsPerCluster; i++) {
            uint64_t currentSector = sector + static_cast<uint64_t>(i);
           
            if (currentSector >= maxSectorCount) {
                std::cerr << "[!] Sector number exceeds device bounds: " << currentSector << std::endl;
                continue;
            }

            if (!readSector(currentSector, sectorBuffer.data(), driveInfo.bytesPerSector)) {
                std::cerr << "[!] Failed to read sector: " << currentSector << std::endl;
                continue;
            }
            processEntriesInSector(entriesPerSector, sectorBuffer);
        }

        uint32_t nextCluster = getNextCluster(cluster);
        if (isValidCluster(nextCluster) && nextCluster != cluster) {
            scanDirectory(nextCluster, depth + 1);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[-] Error in scanDirectory " << e.what() << std::endl;
    }
}

void exFATRecovery::processEntriesInSector(uint32_t entriesPerSector, const std::vector<uint8_t>& sectorBuffer) {
    exFATDirEntryData dirData{};

    for (uint32_t j = 0; j < entriesPerSector; j++) {
        if ((static_cast<uint64_t>(j) + 1) * sizeof(DirectoryEntryCommon) > sectorBuffer.size()) {
            std::cerr << "Buffer overflow prevented in processEntriesInSector " << j << std::endl;
            break;
        }

        const DirectoryEntryCommon* entry = reinterpret_cast<const DirectoryEntryCommon*>(sectorBuffer.data() + j * sizeof(DirectoryEntryCommon));
        uint8_t entryType = entry->EntryType;

        if (IsDirectoryEntry(entryType) && dirData.inFileEntry) {
            finalizeDirectoryEntry(dirData);
        }

        try {
            processDirectoryEntry(entry, dirData);
        }
        catch (const std::exception& e) {
            std::cerr << "Error processing directory entry: " << e.what() << std::endl;
            dirData = {};
            continue;
        }
    }
    if (dirData.inFileEntry) {
        finalizeDirectoryEntry(dirData);
    }
}

void exFATRecovery::processDirectoryEntry(const DirectoryEntryCommon* entry, exFATDirEntryData& dirData) {
    uint8_t entryType = entry->EntryType;

    if (!IsEntryInUse(entryType)) {
        dirData.isDeleted = true;
        if (IsFileNameEntry(entryType)) {
            const FileNameEntry* fnEntry = reinterpret_cast<const FileNameEntry*>(entry);
            dirData.longFilename += extractFileName(fnEntry);
            dirData.inFileEntry = true;
        }
        else if (IsStreamExtensionEntry(entryType)) {
            const StreamExtensionEntry* streamEntry = reinterpret_cast<const StreamExtensionEntry*>(entry);
            dirData.startingCluster = streamEntry->FirstCluster;
            dirData.fileSize = streamEntry->DataLength;
        }
        else if (IsDirectoryEntry(entryType)) {
            const DirectoryEntryExFAT* dirEntry = reinterpret_cast<const DirectoryEntryExFAT*>(entry);
            dirData.isDirectory = (dirEntry->FileAttributes & 0x10) != 0;
        }
    }
}

void exFATRecovery::finalizeDirectoryEntry(exFATDirEntryData& dirData) {
    if (dirData.inFileEntry && !dirData.longFilename.empty() && dirData.startingCluster > 0) {
        if (isValidDeletedEntry(dirData.startingCluster, dirData.fileSize)) {
            try {
                if (dirData.isDirectory) {
                    scanDirectory(dirData.startingCluster, currentRecursionDepth + 1);
                }
                else if (dirData.isDeleted) {
                    exFATFileInfo fileInfo = parseFileInfo(dirData);
                    addToRecoveryList(fileInfo);

                    utils.logFileInfo(fileInfo.fileId, fileInfo.fileName, fileInfo.fileSize);
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error processing entry: " << e.what() << std::endl;
            }
        }
    }
    dirData = {};
}

exFATFileInfo exFATRecovery::parseFileInfo(const exFATDirEntryData& dirData) {
    exFATFileInfo fileInfo = {};
    fileInfo.fileId = this->fileId;
    fileInfo.fileName = dirData.longFilename;
    fileInfo.fileSize = dirData.fileSize;
    fileInfo.cluster = dirData.startingCluster;
    ++this->fileId;
    return fileInfo;
}

std::wstring exFATRecovery::extractFileName(const FileNameEntry* fnEntry) const {
    std::wstring fileName;
    for (int i = 0; i < sizeof(fnEntry->FileName) / 2; i++) {
        wchar_t character = fnEntry->FileName[i];
        if (character == 0x0000) break;  // Null-terminated character sequence
        fileName += character;
    }
    return fileName;
}

void exFATRecovery::addToRecoveryList(const exFATFileInfo& fileInfo) {
    if (config.recover || config.analyze) {
        recoveryList.push_back(fileInfo);
    }
}



/* Corruption analysis */
// Check if cluster is marked as in use in the FAT
bool exFATRecovery::isClusterInUse(uint32_t cluster) {
    uint32_t fatValue = getNextCluster(cluster);
    return (fatValue != 0 && fatValue != 0xF8FFFFFF);
}
// Analyzes clusters for repetition, gaps, backward jumps and calculates the fragmentation score
void exFATRecovery::analyzeClusterPattern(const std::vector<uint32_t>& clusters, exFATRecoveryStatus& status) const {
    //ClusterAnalysisResult result = { 0.0, false, 0, 0, 0 };

    if (clusters.size() < MINIMUM_CLUSTERS_FOR_ANALYSIS) {
        return; // Too few clusters to make reliable assessment
    }

    uint32_t totalAnomalies = 0;
    double avgGapSize = 0;
    uint32_t gapCount = 0;

    for (size_t i = 1; i < clusters.size(); i++) {
        uint32_t gap = 0;

        // Check for repeated clusters
        if (clusters[i] == clusters[i - 1]) {
            //status.isCorrupted = true;
            status.repeatedClusters++;
            totalAnomalies++;
            continue;
        }

        // Calculate gap or detect backward jump
        if (clusters[i] > clusters[i - 1]) {
            gap = clusters[i] - clusters[i - 1] - 1;
        }
        else {
            //status.isCorrupted = true;
            status.backJumps++;
            totalAnomalies++;
            continue;
        }

        // Analyze gaps
        if (gap > 0) {
            avgGapSize += gap;
            gapCount++;

            if (gap >= LARGE_GAP_THRESHOLD) {
                //status.isCorrupted = true;
                status.largeGaps++;
                totalAnomalies++;
            }
        }
    }

    // Calculate average gap size if gaps exist
    if (gapCount > 0) {
        avgGapSize /= gapCount;
    }

    // Calculate overall fragmentation score (0.0 - 1.0)
    double totalPairs = clusters.size() - 1.0;
    status.fragmentation = (std::min)(1.0, static_cast<double>(totalAnomalies) / totalPairs);

    status.hasLargeGaps = (status.largeGaps > totalPairs * SUSPICIOUS_PATTERN_THRESHOLD);
    status.hasBackJumps = (status.backJumps > totalPairs * SUSPICIOUS_PATTERN_THRESHOLD);
    status.hasFragmentedClusters = (status.fragmentation > SEVERE_PATTERN_THRESHOLD);
    status.hasRepeatedClusters = (status.repeatedClusters > 0);

    if (status.hasBackJumps || status.hasFragmentedClusters || status.hasLargeGaps || status.hasRepeatedClusters) status.isCorrupted = true;

}
// Checks if a filename is corrupted
bool exFATRecovery::isFileNameCorrupted(const std::wstring& filename) const {
    if (filename.empty()) return true;

    const std::wstring invalidChars = L"<>:\"/\\|?*";
    if (filename.find_first_of(invalidChars) != std::wstring::npos) return true;

    int controlCharCount = 0;
    int unusualCharCount = 0;

    for (wchar_t c : filename) {
        if (c < 32) controlCharCount++;
        if (c > 127) unusualCharCount++;
    }

    // flag as corrupted if half of the filename contains suspicious characters
    return (controlCharCount > 0 || unusualCharCount > static_cast<int>(filename.length() / 2));
}
// Calculate the percentage of deleted file being overwritten by another deleted file
OverwriteAnalysis exFATRecovery::analyzeClusterOverwrites(uint32_t startCluster, uint64_t expectedSize) {
    OverwriteAnalysis analysis;
    analysis.hasOverwrite = false;
    analysis.overwritePercentage = 0.0;
    

    uint32_t bytesPerCluster = driveInfo.sectorsPerCluster * driveInfo.bytesPerSector;
    uint64_t expectedClusters = (expectedSize + bytesPerCluster - 1) / bytesPerCluster;

    uint32_t currentCluster = startCluster;
    uint64_t currentOffset = 0;

    while (currentOffset < expectedSize && currentCluster >= 2 && currentCluster < 0x0FFFFFF8) {
        auto overlaps = clusterHistory.findOverlappingUsage(currentCluster);

        if (!overlaps.empty()) {
            analysis.hasOverwrite = true;
            analysis.overwrittenClusters.push_back(currentCluster);

            for (const auto& overlap : overlaps) {
                analysis.overwrittenBy[currentCluster].push_back(overlap.second.fileId);
            }
        }

        clusterHistory.recordClusterUsage(currentCluster, nextFileId, currentOffset);

        currentOffset += bytesPerCluster;
        currentCluster = getNextCluster(currentCluster);
    }

    if (!analysis.overwrittenClusters.empty()) {
        analysis.overwritePercentage =
            static_cast<double>(analysis.overwrittenClusters.size()) / expectedClusters * 100.0;
    }

    nextFileId++;
    return analysis;
}


/* Recovery */
std::vector<exFATFileInfo> exFATRecovery::selectFilesToRecover(const std::vector<exFATFileInfo>& recoveryList) {
    char userResponse;
    std::cout << "Options:" << std::endl;
    std::cout << "  1. Process all files" << std::endl;
    std::cout << "  2. Choose specific file(s) to process" << std::endl;
    std::cout << "  0. Exit without processing" << std::endl;

    while (true) {

        std::cout << "\nEnter your option: ";
        std::cin >> userResponse;
        userResponse = toupper(userResponse);
        if (userResponse == '0') exit(0);
        else if (userResponse == '1') return recoveryList;
        else if (userResponse == '2') {
            std::string fileIds;
            std::cout << "\nEnter file IDs to recover (e.g., 1,2,3): ";
            std::cin >> fileIds;

            std::vector<int> fileIdVector;
            std::stringstream ss(fileIds);
            std::string id;
            while (std::getline(ss, id, ',')) {
                try {
                    fileIdVector.push_back(std::stoi(id));
                }
                catch (const std::invalid_argument&) {
                    std::cerr << "\nInvalid input. Please enter numeric IDs.\n";
                    return recoveryList;
                }
            }
            // Create a new vector to store the selected files
            std::vector<exFATFileInfo> selectedFiles;

            // Iterate through the original deletedFiles vector and add the selected files to the new vector
            for (const auto& item : recoveryList) {
                if (std::find(fileIdVector.begin(), fileIdVector.end(), item.fileId) != fileIdVector.end()) {
                    selectedFiles.push_back(item);
                }
            }

            return selectedFiles;
        }
        std::cerr << "Incorrect value" << std::endl;
    }
    return recoveryList;
}

void exFATRecovery::runLogicalDriveRecovery() {
    scanForDeletedFiles();
    recoverPartition();
}

void exFATRecovery::recoverPartition() {
    utils.printHeader("File Recovery and Analysis:");
    if (recoveryList.empty()) {
        if (config.inputFolder.empty()) {
            std::wcerr << "[-] Could not find any deleted files in \"" << config.inputFolder << "\"" << std::endl;
            return;
        }
        if (config.recover || config.analyze) {
            std::cerr << "[-] No deleted files found" << std::endl;
        }
        else std::cout << "Recovery or analysis is disabled. Use --recover or --analyze to proceed." << std::endl;
        return;
    }



    std::vector<exFATFileInfo> selectedDeletedFiles;
    if (!config.targetCluster && !config.targetFileSize) {
        selectedDeletedFiles = selectFilesToRecover(recoveryList);
        utils.printItemDivider();
    }
    else {
        selectedDeletedFiles = recoveryList;
    }

    for (const auto& file : selectedDeletedFiles) {
        processFileForRecovery(file);
    }
}

// Processes each file for recovery based on config options
void exFATRecovery::processFileForRecovery(const exFATFileInfo& fileInfo) {
    bool isExtensionPredicted = false;

    // Skip files that don't match the target cluster and size (if specified)
    if (fileInfo.fileSize <= 0 || (config.targetCluster && config.targetFileSize && (fileInfo.cluster != config.targetCluster || fileInfo.fileSize != config.targetFileSize))) {
        return;
    }


    fs::path outputPath = utils.getOutputPath(fileInfo.fileName, config.outputFolder);
    uint64_t expectedSize = fileInfo.fileSize;

    exFATRecoveryStatus status = {};
    

    uint64_t bytesPerCluster = static_cast<uint64_t>(driveInfo.sectorsPerCluster) * static_cast<uint64_t>(driveInfo.bytesPerSector);
    status.expectedClusters = (expectedSize + bytesPerCluster - 1) / bytesPerCluster;

    std::wcout << "[*] Current file: " << outputPath.filename() << " cluster " << fileInfo.cluster << " (" << expectedSize << " bytes)" << std::endl;
    std::vector<uint32_t> clusterChain;


    validateClusterChain(status, fileInfo.cluster, clusterChain, expectedSize, outputPath, isExtensionPredicted);


    if (config.recover) {
        recoverFile(clusterChain, status, outputPath, expectedSize);
    }
    utils.printItemDivider();
}
// Validates cluster chain and finds potential signs of corruption
void exFATRecovery::validateClusterChain(exFATRecoveryStatus& status, const uint32_t startCluster, std::vector<uint32_t>& clusterChain, uint64_t expectedSize, const fs::path& outputPath, bool isExtensionPredicted){
    if (config.analyze) std::cout << "[*] Analyzing file clusters..." << std::endl;

    uint32_t currentCluster = startCluster;
    std::set<uint32_t> usedClusters;

    while (clusterChain.size() < status.expectedClusters && currentCluster >= 2 && currentCluster < 0x0FFFFFF8) {
        clusterChain.push_back(currentCluster);

        if (config.analyze) {
            // Check for cluster reuse
            if (usedClusters.find(currentCluster) != usedClusters.end()) {
                status.isCorrupted = true;
                status.hasOverwrittenClusters = true;
                status.problematicClusters.push_back(currentCluster);
            }
            usedClusters.insert(currentCluster);

            // Check if cluster is marked as in use by another file
            if (isClusterInUse(currentCluster)) {
                status.isCorrupted = true;
                status.hasOverwrittenClusters = true;
                status.problematicClusters.push_back(currentCluster);
            }
        }

        uint32_t nextCluster = getNextCluster(currentCluster);

        if (nextCluster == currentCluster || nextCluster < 2 || nextCluster >= 0x0FFFFFF8) {
            nextCluster = currentCluster + 1;
        }
        currentCluster = nextCluster;
    }

    if (config.analyze) {
        auto overwriteAnalysis = analyzeClusterOverwrites(startCluster, expectedSize);
        status.hasOverwrittenClusters = overwriteAnalysis.hasOverwrite;
        if (status.hasOverwrittenClusters) status.isCorrupted = true;

        status.hasInvalidFileName = isFileNameCorrupted(outputPath.filename().wstring());
        if (status.hasInvalidFileName) {
            status.isCorrupted;
        }

        if (!isValidCluster(startCluster)) {
            status.isCorrupted = true;
            std::cout << "  [-] Invalid starting cluster: 0x" << std::hex << startCluster << std::dec << std::endl;
        }

        // Check if extension was either missing or had invalid characters
        if (isExtensionPredicted) {
            status.isCorrupted = true;
            status.hasInvalidExtension = true;
        }

        analyzeClusterPattern(clusterChain, status);
        showAnalysisResult(status);
    }

}

void exFATRecovery::recoverFile(const std::vector<uint32_t>& clusterChain, exFATRecoveryStatus& status, const fs::path& outputPath, const uint64_t expectedSize) {
    std::cout << "[*] Recovering file..." << std::endl;
    std::ofstream outputFile(outputPath, std::ios::binary);
    if (!outputFile) {
        throw std::runtime_error("[-] Failed to create output file.");
    }
    // Recovery
    std::vector<uint8_t> sectorBuffer(driveInfo.bytesPerSector);
    for (uint32_t cluster : clusterChain) {
        uint32_t sector = clusterToSector(cluster);

        for (uint64_t i = 0; i < driveInfo.sectorsPerCluster; ++i) {
            
            if (!readSector(static_cast<uint64_t>(sector) + i, sectorBuffer.data(), driveInfo.bytesPerSector)) {
                continue;
            }

            uint64_t bytesToWrite = (std::min)(
                static_cast<uint64_t>(driveInfo.bytesPerSector),
                expectedSize - status.recoveredBytes
                );

            outputFile.write(reinterpret_cast<char*>(sectorBuffer.data()), bytesToWrite);
            status.recoveredBytes += bytesToWrite;
            utils.showProgress(status.recoveredBytes, expectedSize);

            if (status.recoveredBytes >= expectedSize) break;
        }
        status.recoveredClusters++;
        if (status.recoveredBytes >= expectedSize) break;
    }
    outputFile.close();

    showRecoveryResult(status, outputPath, expectedSize);
}

/* Recovery and analysis results */
void exFATRecovery::showRecoveryResult(const exFATRecoveryStatus& status, const fs::path& outputPath, const uint64_t expectedSize) const {
    std::cout << "\n  [*] Clusters recovered: " << status.recoveredClusters
        << " / " << status.expectedClusters << std::endl;
    std::cout << "  [*] Bytes recovered: " << status.recoveredBytes
        << " / " << expectedSize << std::endl;
    if (fs::exists(fs::absolute(outputPath))) {
        std::wcout << "  [+] File saved to " << fs::absolute(outputPath) << L"\n";
    }
    else std::wcout << "  [-] Failed to save file" << std::endl;


}
void exFATRecovery::showAnalysisResult(const exFATRecoveryStatus& status) const {
    if (status.isCorrupted) {
        // std::cout << "\n[*] Recovery Status:" << std::endl;
        std::cout << "  [-] Warning: File appears to be corrupted" << std::endl;

        if (status.hasInvalidFileName) {
            std::cout << "  [-] Filename is corrupted or invalid" << std::endl;
        }
        if (status.hasInvalidExtension) {
            std::cout << "  [-] File extension was either missing or contained invalid characters" << std::endl;
        }

        // Overwritten by other files
        if (status.hasOverwrittenClusters) {
            std::cout << "  [-] Some clusters may have been overwritten" << std::endl;
            std::cout << "  [-] Problematic clusters: ";

            for (auto cluster : status.problematicClusters) {
                std::cout << "0x" << std::hex << cluster << " ";
            }
            std::cout << std::dec << std::endl;
        }


        if (status.hasFragmentedClusters) {
            std::cout << "  [-] Some clusters are fragmented" << std::endl;
            std::cout << "      - Fragmentation score: "
                << std::fixed << std::setprecision(2)
                << (status.fragmentation) << "%" << std::endl;
        }
        if (status.hasRepeatedClusters) {
            std::cout << "  [-] Repeated clusters found: "
                << status.repeatedClusters << std::endl;
        }
        if (status.hasBackJumps) {
            std::cout << "  [-] Backward jumps detected: "
                << status.backJumps << std::endl;
        }
        if (status.hasLargeGaps) {
            std::cout << "  [-] Large gaps detected: "
                << status.largeGaps << std::endl;
        }
    }
    else std::cout << "  [+] No signs of corruption found " << std::endl;
}


/* Public */

/* Entry point */
void exFATRecovery::startRecovery() {
    if (this->driveType == DriveType::LOGICAL_TYPE) runLogicalDriveRecovery();
    else {
        throw std::runtime_error("Unknown drive type.");
    }

}


