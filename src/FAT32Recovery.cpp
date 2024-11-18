#include "IConfigurable.h"
#include "FAT32Recovery.h"
#include "LogicalDriveReader.h"
#include <set>
#include <vector>
#include <cwctype>
#include <codecvt>
#include <iostream>


// Constructor
FAT32Recovery::FAT32Recovery(const DriveType& driveType, std::unique_ptr<SectorReader> reader)
    : IConfigurable(), driveType(driveType)
{
    printToolHeader();

    utils.ensureOutputDirectory();
    setSectorReader(std::move(reader));
    readBootSector(0);
}
// Destructor
FAT32Recovery::~FAT32Recovery() {
    utils.closeLogFile();
}

void FAT32Recovery::printToolHeader() const {
    std::cout << "\n";
    std::cout << "\n";
    std::cout << " *************************************************************************\n";
    std::cout << " *  _____ _  _____ _________    ____                                     *\n";
    std::cout << " * |  ___/ \\|_   _|___ /___ \\  |  _ \\ ___  ___ _____   _____ _ __ _   _  *\n";
    std::cout << " * | |_ / _ \\ | |   |_ \\ __) | | |_) / _ \\/ __/ _ \\ \\ / / _ \\ '__| | | | *\n";
    std::cout << " * |  _/ ___ \\| |  ___) / __/  |  _ <  __/ (_| (_) \\ V /  __/ |  | |_| | *\n";
    std::cout << " * |_|/_/   \\_\\_| |____/_____| |_| \\_\\___|\\___\\___/ \\_/ \\___|_|   \\__, | *\n";
    std::cout << " *                                                                |___/  *\n";
    std::cout << " *************************************************************************\n";
    std::cout << "\n";
    std::cout << "\n";
}

void FAT32Recovery::setSectorReader(std::unique_ptr<SectorReader> reader) {
    if (!reader) {
        throw std::runtime_error("Invalid sector reader");
    }
    sectorReader = std::move(reader);
}
void FAT32Recovery::readBootSector(uint32_t sector) {
    uint64_t bytesPerSector = getBytesPerSector();
    std::vector<uint64_t> buffer(bytesPerSector);
    if (!readSector(sector, buffer.data(), bytesPerSector)) {
        throw std::runtime_error("Failed to read FAT32 boot sector");
    }

    driveInfo.bootSector = *reinterpret_cast<BootSector*>(buffer.data());

    if (std::memcmp(this->driveInfo.bootSector.FileSystemType, "FAT32", 5) != 0) {
        throw std::runtime_error("Not a valid FAT32 volume");
    }

    /*if (driveInfo.bootSector.BytesPerSector > sectorBuffer.size()) {
        sectorBuffer.resize(driveInfo.bootSector.BytesPerSector);
    }*/


    driveInfo.fatStartSector = driveInfo.bootSector.ReservedSectorCount;
    driveInfo.dataStartSector = driveInfo.fatStartSector + (driveInfo.bootSector.NumFATs * driveInfo.bootSector.FATSize32);
    driveInfo.rootDirCluster = driveInfo.bootSector.RootCluster;

    uint32_t rootDirSectors = ((driveInfo.bootSector.RootEntryCount * 32) + (driveInfo.bootSector.BytesPerSector - 1)) / driveInfo.bootSector.BytesPerSector;
    uint32_t totalSectors = (driveInfo.bootSector.TotalSectors32 != 0) ? driveInfo.bootSector.TotalSectors32 : driveInfo.bootSector.TotalSectors16;
    uint32_t dataSectors = totalSectors - (driveInfo.bootSector.ReservedSectorCount + (driveInfo.bootSector.NumFATs * driveInfo.bootSector.FATSize32) + rootDirSectors);
    driveInfo.maxClusterCount = dataSectors / driveInfo.bootSector.SectorsPerCluster;
}
uint64_t FAT32Recovery::getBytesPerSector() {
    if (!sectorReader) {
        throw std::runtime_error("Sector reader not initialized");
    }

    uint64_t bytesPerSector = sectorReader->getBytesPerSector();
    if (bytesPerSector == 0) {
        throw std::runtime_error("Invalid bytes per sector");
    }
    return bytesPerSector;
}
bool FAT32Recovery::readSector(uint64_t sector, void* buffer, uint64_t size) {
    return sectorReader && sectorReader->readSector(sector, buffer, size);
}


bool FAT32Recovery::isValidCluster(uint32_t cluster) const {
    // Basic range check
    if (cluster < MIN_DATA_CLUSTER || cluster > driveInfo.maxClusterCount) {
        return false;
    }

    // Check if cluster is in valid range but marked as bad or EOF
    if (cluster >= BAD_CLUSTER) {
        return false;
    }

    return true;
}
uint32_t FAT32Recovery::sanitizeCluster(uint32_t cluster) const {
    // Filter out obviously invalid values
    if (cluster == 0 || cluster < MIN_DATA_CLUSTER ||
        cluster >= BAD_CLUSTER) {
        //std::cerr << "Warning: Invalid cluster number detected: 0x"
        //    << std::hex << cluster << std::dec << std::endl;
        return 0;
    }

    // Check against calculated maximum cluster count
    if (cluster > driveInfo.maxClusterCount) {
        std::cerr << "Warning: Cluster number exceeds maximum count: 0x"
            << std::hex << cluster << " (max: " << driveInfo.maxClusterCount << ")"
            << std::dec << std::endl;
        return 0;
    }

    return cluster;
}
uint32_t FAT32Recovery::clusterToSector(uint32_t cluster) const {
    return driveInfo.dataStartSector + (cluster - 2) * driveInfo.bootSector.SectorsPerCluster;
}
uint32_t FAT32Recovery::getNextCluster(uint32_t cluster) {
    uint32_t fatOffset = cluster * 4;
    uint32_t fatSector = driveInfo.fatStartSector + (fatOffset / driveInfo.bootSector.BytesPerSector);
    uint32_t entryOffset = fatOffset % driveInfo.bootSector.BytesPerSector;

    std::vector<uint8_t> sectorBuffer(driveInfo.bootSector.BytesPerSector);
    if (!readSector(fatSector, sectorBuffer.data(), driveInfo.bootSector.BytesPerSector)) {
        std::cerr << "Error: Failed to read FAT sector " << fatSector << std::endl;
        return 0xFFFFFFFF;
    }

    uint32_t fatEntry = *reinterpret_cast<uint32_t*>(sectorBuffer.data() + entryOffset);
    uint32_t nextCluster = fatEntry & 0x0FFFFFFF;  // Mask the lower 28 bits

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


/*=============== File scan ===============*/
void FAT32Recovery::scanForDeletedFiles(uint32_t startSector) {
    utils.printHeader("File Search:");
    if (!utils.openLogFile() && !utils.confirmProceedWithoutLogFile()) {
        std::cout << "Exitting..." << std::endl;
        exit(1);
    }

    scanDirectory(driveInfo.rootDirCluster);

    utils.closeLogFile();
    utils.printFooter();
}

void FAT32Recovery::scanDirectory(uint32_t cluster, bool isTargetFolder) {
    if (!isValidCluster(cluster)) {
        std::cerr << "Warning: Invalid cluster detected: 0x"
            << std::hex << cluster << std::dec << std::endl;
        return;
    }

    uint32_t sector = clusterToSector(cluster);
    uint32_t entriesPerSector = driveInfo.bootSector.BytesPerSector / sizeof(DirectoryEntry);
    std::vector<uint8_t> sectorBuffer(driveInfo.bootSector.BytesPerSector);
    for (uint64_t i = 0; i < driveInfo.bootSector.SectorsPerCluster; i++) {
        //sectorBuffer.resize(driveInfo.bootSector.BytesPerSector);
        
        if (readSector(static_cast<uint64_t>(sector) + i, sectorBuffer.data(), driveInfo.bootSector.BytesPerSector)) {
            processEntriesInSector(entriesPerSector, isTargetFolder, sectorBuffer);
        }
        else {
            std::cerr << "Warning: Failed to read sector " << (sector + i) << std::endl;
        }
    }

    uint32_t nextCluster = getNextCluster(cluster);
    if (isValidCluster(nextCluster)) {
        scanDirectory(nextCluster, isTargetFolder);
    }
}

void FAT32Recovery::processEntriesInSector(uint32_t entriesPerSector, bool isTargetFolder, std::vector<uint8_t>& sectorBuffer) {
    std::wstring longFilename;
    for (uint32_t j = 0; j < entriesPerSector; j++) {
        DirectoryEntry* entry = reinterpret_cast<DirectoryEntry*>(sectorBuffer.data() + j * sizeof(DirectoryEntry));
        if (entry->Name[0] == 0x00) return; // End of directory

        bool isDeleted = entry->Name[0] == 0xE5;
        if (entry->Attr == 0x0F) { // Long filename
            longFilename = getLongFilename(entry) + longFilename;
            continue;
        }

        std::wstring filename;
        if (!longFilename.empty()) { // LFN
            filename = longFilename;
            longFilename.clear();
        }
        else { // SFN
            filename = getShortFilename(entry, isDeleted);
        }

        processDirectoryEntry(entry,  filename, isTargetFolder);
    }
}

void FAT32Recovery::processDirectoryEntry(const DirectoryEntry* entry, const std::wstring& filename, bool isTargetFolder) {
    if (!entry) return;

    bool isDeleted = entry->Name[0] == 0xE5;
    bool isDirectory = entry->Attr & 0x10 && entry->Name[0] != '.';

    uint32_t subDirCluster = ((uint32_t)entry->FstClusHI << 16) | entry->FstClusLO;
    subDirCluster = sanitizeCluster(subDirCluster);
    if (subDirCluster == 0) return;

    if (isDirectory) {
        scanDirectory(subDirCluster, false);
    }
    else if (isDeleted) {
        uint32_t fileSize = entry->FileSize;
        FAT32FileInfo fileInfo = parseFileInfo(filename, subDirCluster, fileSize);

        addToRecoveryList(fileInfo);
        utils.logFileInfo(fileInfo.fileId, fileInfo.fileName, fileInfo.fileSize);
    }
}

void FAT32Recovery::addToRecoveryList(const FAT32FileInfo& fileInfo) {
    if (config.recover || config.analyze) {
        recoveryList.push_back(fileInfo);
    }
}
// Extract long filename from LFN entry
std::wstring FAT32Recovery::getLongFilename(DirectoryEntry* entry) const {
    LFNEntry* lfn = reinterpret_cast<LFNEntry*>(entry);
    std::wstring lfnPart;
    lfnPart.reserve(13);
    for (int k = 0; k < 5; k++) lfnPart += lfn->Name1[k];
    for (int k = 0; k < 6; k++) lfnPart += lfn->Name2[k];
    for (int k = 0; k < 2; k++) lfnPart += lfn->Name3[k];

    lfnPart.erase(
        std::remove_if(lfnPart.begin(), lfnPart.end(),
            [](wchar_t c) {
                return c == 0 || c == 0xFFFF || c < 32;
            }),
        lfnPart.end()
                );

    return lfnPart;
}
// Extract short filename from Directory Entry
std::wstring FAT32Recovery::getShortFilename(const DirectoryEntry* entry, bool isDeleted) const {
    std::wstring filename;
    char name[9] = { 0 };  // 8 characters for name + null terminator
    char ext[4] = { 0 };   // 3 characters for extension + null terminator

    // Extract the name (first 8 bytes)
    std::memcpy(name, entry->Name, 8);
    // If the file is deleted, restore the first character
    if (isDeleted) {
        name[0] = '_';
    }
    // Trim trailing spaces
    while (name[0] && name[strlen(name) - 1] == ' ') {
        name[strlen(name) - 1] = 0;
    }

    // Extract the extension (last 3 bytes)
    std::memcpy(ext, entry->Name + 8, 3);
    // Trim trailing spaces
    while (ext[0] && ext[strlen(ext) - 1] == ' ') {
        ext[strlen(ext) - 1] = 0;
    }

    // Convert to wide string
    filename = std::wstring(name, name + strlen(name));

    // Add extension if it exists
    if (ext[0]) {
        filename += L'.';
        filename += std::wstring(ext, ext + strlen(ext));
    }

    return filename;
}
// Clean filename and combine it with its extension
FAT32FileInfo FAT32Recovery::parseFileInfo(const std::wstring& fullName, uint32_t startCluster, uint32_t expectedSize) {
    FAT32FileInfo fileInfo = {};
    fileInfo.fileId = fileId;
    fileInfo.fullName = fullName;
    fileInfo.fileName = L"";
    fileInfo.extension = L"";
    fileInfo.fileSize = expectedSize;
    fileInfo.cluster = startCluster;
    fileInfo.isExtensionPredicted = false;
    

    std::wstring cleanName = fullName;
    cleanName.erase(std::remove(cleanName.begin(), cleanName.end(), L'\0'), cleanName.end());

    fileInfo.fullName = cleanName;

    size_t dotPos = fullName.find_last_of(L".");
    if (dotPos != std::wstring::npos && dotPos != 0) {
        fileInfo.fileName = fullName.substr(0, dotPos);
        fileInfo.extension = fullName.substr(dotPos + 1);

        bool isValid = std::all_of(fileInfo.extension.begin(), fileInfo.extension.end(), ::iswalnum);
        if (!isValid) {
            std::wcerr << "  [-] Extension is invalid (" << fileInfo.extension << ") file may be corrupted" << std::endl;
            fileInfo.extension = predictExtension(startCluster, expectedSize);
            fileInfo.isExtensionPredicted = true;
            fileInfo.fullName = fileInfo.fileName + L"." + fileInfo.extension;
        }
    }
    else {
        std::wcerr << "  [-] Extension is missing, file may be corrupted" << std::endl;
        fileInfo.extension = predictExtension(startCluster, expectedSize);
        fileInfo.isExtensionPredicted = true;
        fileInfo.fileName = fullName;
        fileInfo.fullName = fullName + L"." + fileInfo.extension;
    }
    ++fileId;
    return fileInfo;
}
// Compare two filenames (case-insensitive)
bool FAT32Recovery::compareFolderNames(const std::wstring& filename1, const std::wstring& filename2) const {
    std::wstring trimmedFilename = filename1.substr(0, filename1.find_first_of(L'\0'));
    trimmedFilename.erase(std::remove(trimmedFilename.begin(), trimmedFilename.end(), 0xFFFF), trimmedFilename.end());

    // Convert both filenames to uppercase
    std::wstring upper1 = trimmedFilename;
    std::wstring upper2 = filename2;

    std::transform(upper1.begin(), upper1.end(), upper1.begin(), [](wchar_t ch) { return std::towupper(ch); });
    std::transform(upper2.begin(), upper2.end(), upper2.begin(), [](wchar_t ch) { return std::towupper(ch); });

    // Trim trailing spaces and null characters safely
    if (!upper1.empty()) {
        upper1.erase(upper1.find_last_not_of(L' ') + 1);
    }
    if (!upper2.empty()) {
        upper2.erase(upper2.find_last_not_of(L' ') + 1);
    }

    return upper1 == upper2;
}
// Predict file extension based on content
std::wstring FAT32Recovery::predictExtension(const uint32_t cluster, const uint32_t expectedSize) {
    uint32_t bytesPerCluster = driveInfo.bootSector.SectorsPerCluster * driveInfo.bootSector.BytesPerSector;
    uint32_t firstSector = clusterToSector(cluster);


    std::vector<uint8_t> buffer;
    buffer.resize(driveInfo.bootSector.BytesPerSector);

    std::wcout << "  [*] Prediting extension..." << std::endl;

    // Read only the first sector to capture file signature
    bool succ = readSector(firstSector, buffer.data(), driveInfo.bootSector.BytesPerSector);

    std::vector<uint8_t> firstBytes(buffer.begin(), buffer.begin() + 8);

    std::wstring extension = guessFileExtension(getFileSignature(firstBytes));

    // Default to .bin if extension could not be determined
    if (extension == L"bin") {
        std::wcout << "  [-] Couldn't predict the extension. Defaulting to .bin" << std::endl;
    }
    else {
        std::wcout << "  [*] Predicted extension: " << extension << std::endl;
    }

    return extension;
}
// Get file signature from first bytes
std::string FAT32Recovery::getFileSignature(const std::vector<uint8_t>& data) const {
    std::stringstream ss;
    for (int i = 0; i < (std::min)(4, static_cast<int>(data.size())); ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    return ss.str();
}
// Convert file signature to extension
std::wstring FAT32Recovery::guessFileExtension(const std::string& signature) const {
    // Images
    if (signature.substr(0, 6) == "ffd8ff") return L"jpg";          // JPEG/JPG
    if (signature.substr(0, 8) == "89504e47") return L"png";        // PNG
    if ((signature.substr(0, 8) == "47494638") ||
        (signature.substr(0, 3) == "383761")) return L"gif";        // GIF87a/GIF89a
    if (signature.substr(0, 4) == "424d") return L"bmp";            // BMP
    if (signature.substr(0, 8) == "49492a00") return L"tif";        // TIFF
    if (signature.substr(0, 8) == "4d4d002a") return L"tif";        // TIFF
    if (signature.substr(0, 8) == "52494646") return L"webp";       // WebP

    // Documents
    if (signature.substr(0, 8) == "25504446") return L"pdf";        // PDF
    if (signature.substr(0, 8) == "504b0304") return L"zip";        // ZIP/DOCX/XLSX/PPTX
    if (signature.substr(0, 8) == "d0cf11e0") return L"doc";        // DOC/XLS/PPT (Legacy Office)
    if (signature.substr(0, 8) == "7b5c7274") return L"rtf";        // RTF

    // Audio/Video
    if (signature.substr(0, 8) == "52494646") return L"wav";        // WAV
    if (signature.substr(0, 8) == "494433") return L"mp3";          // MP3
    if (signature.substr(0, 8) == "66747970") return L"mp4";        // MP4
    if (signature.substr(0, 8) == "52494646") return L"avi";        // AVI
    if (signature.substr(0, 4) == "4f676753") return L"ogg";        // OGG

    // Executables and Libraries
    if (signature.substr(0, 4) == "4d5a") return L"exe";            // EXE/DLL
    if (signature.substr(0, 8) == "7f454c46") return L"elf";        // ELF (Linux executables)

    // Archives
    if (signature.substr(0, 8) == "504b0304") return L"zip";        // ZIP
    if (signature.substr(0, 6) == "526172") return L"rar";          // RAR
    if (signature.substr(0, 8) == "1f8b0808") return L"gz";         // GZIP
    if (signature.substr(0, 6) == "425a68") return L"bz2";          // BZIP2
    if (signature.substr(0, 8) == "377abcaf") return L"7z";         // 7Z

    // Database
    if (signature.substr(0, 8) == "53514c69") return L"sqlite";     // SQLite

    // Programming
    if (signature.substr(0, 8) == "3c3f786d") return L"xml";        // XML
    if (signature.substr(0, 8) == "7b0d0a20") return L"json";       // JSON
    if (signature.substr(0, 8) == "3c21444f") return L"html";       // HTML

    // Font files
    if (signature.substr(0, 8) == "4f54544f") return L"otf";        // OTF
    if (signature.substr(0, 8) == "00010000") return L"ttf";        // TTF

    // If no match is found
    return L"bin";
}



/*=============== Corruption analysis ===============*/
// Check if cluster is marked as in use in the FAT
bool FAT32Recovery::isClusterInUse(uint32_t cluster) {
    uint32_t fatValue = getNextCluster(cluster);
    return (fatValue != 0 && fatValue != 0xF8FFFFFF);
}
// Analyzes clusters for repetition, gaps, backward jumps and calculates the fragmentation score
void FAT32Recovery::analyzeClusterPattern(const std::vector<uint32_t>& clusters, RecoveryStatus& status) const {
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
bool FAT32Recovery::isFileNameCorrupted(const std::wstring& filename) const {
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
OverwriteAnalysis FAT32Recovery::analyzeClusterOverwrites(uint32_t startCluster, uint32_t expectedSize) {
    OverwriteAnalysis analysis = {
        .hasOverwrite = false,
        .overwritePercentage = 0.0
    };

    uint32_t bytesPerCluster = driveInfo.bootSector.SectorsPerCluster * driveInfo.bootSector.BytesPerSector;
    uint32_t expectedClusters = (expectedSize + bytesPerCluster - 1) / bytesPerCluster;

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


/*=============== Recovery ===============*/
// Asks user to either recover all files or only the selected IDs
std::vector<FAT32FileInfo> FAT32Recovery::selectFilesToRecover(const std::vector<FAT32FileInfo>& recoveryList) {
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
            std::vector<FAT32FileInfo> selectedFiles;

            // Iterate through the original recoveryList vector and add the selected files to the new vector
            for (const auto& file : recoveryList) {
                if (std::find(fileIdVector.begin(), fileIdVector.end(), file.fileId) != fileIdVector.end()) {
                    selectedFiles.push_back(file);
                }
            }

            return selectedFiles;
        }
        std::cerr << "Incorrect value" << std::endl;
    }
    return recoveryList;
}
// Recover all found deleted files or a specific file if target cluster and target filesize is specified
void FAT32Recovery::recoverPartition() {
    utils.printHeader("File Recovery and Analysis:");
    if (recoveryList.empty()) {
        if (config.recover || config.analyze)  std::cerr << "[-] No deleted files found" << std::endl;
        else std::cout << "[!] Recovery or analysis is disabled. Use --recover and/or --analyze to proceed." << std::endl;

        return;
    }
    std::vector<FAT32FileInfo> selectedDeletedFiles;
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
void FAT32Recovery::processFileForRecovery(const FAT32FileInfo& fileInfo) {
    bool isExtensionPredicted = false;

    // Skip files that don't match the target cluster and size (if specified)
    if (fileInfo.fileSize <= 0 || (config.targetCluster && config.targetFileSize && (fileInfo.cluster != config.targetCluster || fileInfo.fileSize != config.targetFileSize))) {
        return;  
    }
    
    
    fs::path outputPath = utils.getOutputPath(fileInfo.fullName, config.outputFolder);
    
    if (fileInfo.fileSize > UINT32_MAX) {
        throw std::overflow_error("File size exceeds 32-bit limit!");
    }
    uint32_t expectedSize = static_cast<uint32_t>(fileInfo.fileSize);

    RecoveryStatus status = {};

    uint32_t bytesPerCluster = driveInfo.bootSector.SectorsPerCluster * driveInfo.bootSector.BytesPerSector;
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
void FAT32Recovery::validateClusterChain(RecoveryStatus& status, const uint32_t startCluster, std::vector<uint32_t>& clusterChain, uint32_t expectedSize, const fs::path& outputPath, bool isExtensionPredicted){
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
            //status.hasFragmentedClusters = true;
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
// Recovers specific file
void FAT32Recovery::recoverFile(const std::vector<uint32_t>& clusterChain, RecoveryStatus& status, const fs::path& outputPath, const uint32_t expectedSize) {
    std::cout << "[*] Recovering file..." << std::endl;
    std::ofstream outputFile(outputPath, std::ios::binary);
    if (!outputFile) {
        throw std::runtime_error("[-] Failed to create output file.");
    }
    // Recovery
    std::vector<uint8_t> sectorBuffer(driveInfo.bootSector.BytesPerSector);
    for (uint32_t cluster : clusterChain) {
        uint32_t sector = clusterToSector(cluster);

        for (uint64_t i = 0; i < driveInfo.bootSector.SectorsPerCluster; ++i) {
            if (!readSector(static_cast<uint64_t>(sector) + i, sectorBuffer.data(), driveInfo.bootSector.BytesPerSector)) {
                continue;
            }

            uint64_t bytesToWrite = (std::min)(
                static_cast<uint64_t>(driveInfo.bootSector.BytesPerSector),
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
void FAT32Recovery::showAnalysisResult(const RecoveryStatus& status) const {
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
void FAT32Recovery::showRecoveryResult(const RecoveryStatus& status, const fs::path& outputPath, const uint32_t expectedSize) const {
    std::cout << "\n  [*] Clusters recovered: " << status.recoveredClusters
        << " / " << status.expectedClusters << std::endl;
    std::cout << "  [*] Bytes recovered: " << status.recoveredBytes
        << " / " << expectedSize << std::endl;
    if (fs::exists(fs::absolute(outputPath))) {
        std::wcout << "  [+] File saved to " << fs::absolute(outputPath) << L"\n";
    } 
    else std::wcout << "  [-] Failed to save file" << std::endl;
        
    
}


void FAT32Recovery::runLogicalDriveRecovery() {
    scanForDeletedFiles(driveInfo.rootDirCluster);
    recoverPartition();
}

/*=============== Public Interface ===============*/

/* Recovery entry point */
void FAT32Recovery::startRecovery() {
    if (this->driveType == DriveType::LOGICAL_TYPE) runLogicalDriveRecovery();
    else {
        throw std::runtime_error("Unknown drive type.");
    }

}