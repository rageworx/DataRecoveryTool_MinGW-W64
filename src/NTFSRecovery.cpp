#include "NTFSRecovery.h"
#include <memory>
#include <set>


NTFSRecovery::NTFSRecovery(const DriveType& driveType, std::unique_ptr<SectorReader> reader) : IConfigurable(), driveType(driveType) {
    printToolHeader();
    utils.ensureOutputDirectory();
    setSectorReader(std::move(reader));
    readBootSector(0);
}
NTFSRecovery::~NTFSRecovery() {
    utils.closeLogFile();
}

void NTFSRecovery::printToolHeader() const {
    std::cout << "\n\n";
    std::cout << " ***********************************************************************\n";
    std::cout << " *  _   _ _____ _____ ____    ____                                     *\n";
    std::cout << " * | \\ | |_   _|  ___/ ___|  |  _ \\ ___  ___ _____   _____ _ __ _   _  *\n";
    std::cout << " * |  \\| | | | | |_  \\___ \\  | |_) / _ \\/ __/ _ \\ \\ / / _ \\ '__| | | | *\n";
    std::cout << " * | |\\  | | | |  _|  ___) | |  _ <  __/ (_| (_) \\ V /  __/ |  | |_| | *\n";
    std::cout << " * |_| \\_| |_| |_|   |____/  |_| \\_\\___|\\___\\___/ \\_/ \\___|_|   \\__, | *\n";
    std::cout << " *                                                              |___/  *\n";
    std::cout << " ***********************************************************************\n";
    std::cout << "\n\n";
}

void NTFSRecovery::setSectorReader(std::unique_ptr<SectorReader> reader) {
    if (!reader) {
        throw std::runtime_error("Invalid sector reader");
    }
    sectorReader = std::move(reader);
}

bool NTFSRecovery::readSector(uint64_t sector, void* buffer, uint32_t size) {
    return sectorReader && sectorReader->readSector(sector, buffer, size);
}

void NTFSRecovery::readBootSector(uint64_t sector) {
    uint32_t bytesPerSector = getBytesPerSector();

    
    std::vector<uint8_t> buffer(bytesPerSector);
    
    if (!readSector(sector, buffer.data(), bytesPerSector)) {
        throw std::runtime_error("Failed to read NTFS boot sector");
    }

    driveInfo.bootSector = *reinterpret_cast<NTFSBootSector*>(buffer.data());
    
    if (memcmp(driveInfo.bootSector.oemID, "NTFS", 4) != 0) {
        throw std::runtime_error("Not a valid NTFS volume");
    }

    if (bytesPerSector > driveInfo.bootSector.bytesPerSector) {
        driveInfo.bytesPerSector = bytesPerSector;
    }
    else {
        driveInfo.bytesPerSector = static_cast<uint32_t>(driveInfo.bootSector.bytesPerSector);
    }



    driveInfo.bytesPerCluster = driveInfo.bootSector.bytesPerSector * driveInfo.bootSector.sectorsPerCluster;

    // Calculate MFT record size
    // If clustersPerMftRecord is positive, it represents clusters per record
    // If negative, it represents 2^(-1 * clustersPerMftRecord) bytes
    if (driveInfo.bootSector.clustersPerMftRecord > 0) {
        driveInfo.mftRecordSize = driveInfo.bootSector.clustersPerMftRecord * driveInfo.bytesPerCluster;
    }
    else {
        driveInfo.mftRecordSize = 1 << (-1 * driveInfo.bootSector.clustersPerMftRecord);
    }

    driveInfo.mftOffset = driveInfo.bootSector.mftCluster * driveInfo.bytesPerCluster;
}

uint32_t NTFSRecovery::getBytesPerSector() {
    if (!sectorReader) {
        throw std::runtime_error("Sector reader not initialized");
    }

    uint32_t bytesPerSector = sectorReader->getBytesPerSector();
    if (bytesPerSector == 0) {
        throw std::runtime_error("Invalid bytes per sector");
    }
    return bytesPerSector;
}

uint64_t NTFSRecovery::getTotalMftRecords() {
    return sectorReader->getTotalMftRecords();
}

uint32_t NTFSRecovery::getSectorsPerMftRecord() {
    uint32_t sectorsPerMftRecord = driveInfo.mftRecordSize / driveInfo.bootSector.bytesPerSector;
    if (driveInfo.mftRecordSize % driveInfo.bootSector.bytesPerSector != 0) {
        sectorsPerMftRecord++;
    }
    return sectorsPerMftRecord;
}


uint64_t NTFSRecovery::clusterToSector(uint64_t cluster) const {
    return static_cast<uint64_t>(cluster) *
        static_cast<uint64_t>(driveInfo.bootSector.sectorsPerCluster);
}

bool NTFSRecovery::isValidSector(uint64_t mftSector) const {
    if (mftSector >= driveInfo.bootSector.totalSectors) {
        std::cerr << "Error: Calculated mftSector (" << mftSector
            << ") out of bounds (total sectors: "
            << driveInfo.bootSector.totalSectors << ")" << std::endl;
        return false;
    }
    return true;
}

bool NTFSRecovery::isValidFileRecord(const MFTEntryHeader* entry) const {
    // Verify "FILE" signature (0x46494C45) in little endian
    return entry->signature == 0x454C4946;
}

bool NTFSRecovery::validateFileInfo(const NTFSFileInfo& fileInfo) const {
    if (!fileInfo.fileName.empty() && fileInfo.fileSize == 0) {
        std::wcerr << L"[-] File \"" << fileInfo.fileName << "\" has invalid size: " << fileInfo.fileSize << " bytes." << std::endl;
        return false;
    }
    if (fileInfo.fileName.empty() || fileInfo.fileSize == 0) {
        //std::cerr << "Filename or Filesize was not found." << std::endl;
        return false;
    }

    if (fileInfo.nonResident && (fileInfo.cluster == 0 || fileInfo.runLength == 0)) {
        //std::cerr << "Data not found in non resident file." << std::endl;
        return false;
    }

    if (!fileInfo.nonResident && fileInfo.data.empty()) {
        //std::cerr << "Cluster or run length not found in non resident file." << std::endl;
        return false;
    }
    return true;
}

void NTFSRecovery::clearFileInfo(NTFSFileInfo& fileInfo) const {
    fileInfo.cluster = 0;
    fileInfo.fileId = 0;
    fileInfo.fileName = L"";
    fileInfo.fileSize = 0;
    fileInfo.nonResident = false;
    fileInfo.runLength = 0;
    fileInfo.data.clear();
}


/* File scan */
bool NTFSRecovery::readMftRecord(std::vector<uint8_t>& mftBuffer, const uint32_t sectorsPerMftRecord, const uint64_t currentSector) {
    for (uint32_t i = 0; i < sectorsPerMftRecord; i++) {
        if (!readSector(currentSector + i,
            mftBuffer.data() + (static_cast<uint64_t>(i) * driveInfo.bootSector.bytesPerSector),
            driveInfo.bootSector.bytesPerSector)) {
            std::cerr << "Failed to read MFT sector " << (currentSector + i) << std::endl;
            return false;
        }
    }
    return true;
}

void NTFSRecovery::scanForDeletedFiles() {
    utils.printHeader("File Search:");

    if (!utils.openLogFile() && !utils.confirmProceedWithoutLogFile()) {
        std::cout << "Exitting..." << std::endl;
        exit(1);
    }

    scanMFT();
    utils.closeLogFile();
    utils.printFooter();
}

void NTFSRecovery::scanMFT() {
    uint64_t mftSector = clusterToSector(driveInfo.bootSector.mftCluster);
    if (!isValidSector(mftSector)) return;

    uint32_t sectorsPerMftRecord = getSectorsPerMftRecord();

    std::vector<uint8_t> mftBuffer(driveInfo.mftRecordSize);

    uint64_t totalMftRecords = getTotalMftRecords();
    for (uint64_t recordIndex = 0; recordIndex < totalMftRecords; recordIndex++) {
        uint64_t currentSector = mftSector + (recordIndex * sectorsPerMftRecord);

        if (!readMftRecord(mftBuffer, sectorsPerMftRecord, currentSector)) {
            continue;
        }

        processMftRecord(mftBuffer);
    }
}

void NTFSRecovery::processMftRecord(std::vector<uint8_t>& mftBuffer) {
    try {
        // Process the complete MFT record
        const MFTEntryHeader* entry = reinterpret_cast<const MFTEntryHeader*>(mftBuffer.data());

        if (!isValidFileRecord(entry)) return;

        // Check if record is in use
        bool isDeleted = (entry->flags & 0x0001) == 0;
        if (!isDeleted) return;

        NTFSFileInfo fileInfo = {};
        uint32_t attributeOffset = entry->firstAttributeOffset;
        bool hasFileName = false;
        bool hasData = false;

        processAttribute(mftBuffer, fileInfo, attributeOffset, hasFileName, hasData, isDeleted);

        if (!hasFileName && !hasData) return;

        
        try {
            if (validateFileInfo(fileInfo)) {
                addToRecoveryList(fileInfo);
                utils.logFileInfo(fileInfo.fileId, fileInfo.fileName, fileInfo.fileSize);
                this->fileId++;
            }
            clearFileInfo(fileInfo);
        }
        catch (const std::exception& e) {
            std::cerr << "\nException while validating file info or adding to recovery list: " << e.what() << std::endl;
            return;
        }
        
    }
    catch (const std::exception& e) {
        std::cerr << "\nException while processing record: " << e.what() << std::endl;
        return;
    }
}

void NTFSRecovery::processAttribute(const std::vector<uint8_t>& mftBuffer, NTFSFileInfo& fileInfo, uint32_t attributeOffset, bool& hasFileName, bool& hasData, bool isDeleted) {
    while (attributeOffset < driveInfo.mftRecordSize) {
        const AttributeHeader* attr = reinterpret_cast<const AttributeHeader*>(
            mftBuffer.data() + attributeOffset);

        // Check for end marker
        if (attr->type == 0xFFFFFFFF) break;

        // Validate attribute length
        if (attr->length == 0 || attributeOffset + attr->length > driveInfo.mftRecordSize) {
            break;
        }

        // Process different attribute types
        switch (attr->type) {
        case 0x30:  // $FILE_NAME
            processFileNameAttribute(attr, mftBuffer.data() + attributeOffset, isDeleted, fileInfo);
            hasFileName = true;
            break;

        case 0x80:  // $DATA
            processDataAttribute(attr, mftBuffer.data() + attributeOffset, isDeleted, fileInfo);
            hasData = true;
            break;
        }

        attributeOffset += attr->length;
    }
}

void NTFSRecovery::processFileNameAttribute(const AttributeHeader* attr, const uint8_t* attrData, bool isDeleted, NTFSFileInfo& fileInfo) {
    if (!attr->nonResident) {  // File name is always resident
        const ResidentAttributeHeader* resAttr = reinterpret_cast<const ResidentAttributeHeader*>(attrData);
        const uint8_t* filenameData = attrData + resAttr->contentOffset;
        const FileNameAttribute* fnAttr = reinterpret_cast<const FileNameAttribute*>(filenameData);

        if (fnAttr->nameLength > 255) return;
        std::wstring wfilename(fnAttr->name, fnAttr->nameLength);

        if (isDeleted) {
            fileInfo.fileName = wfilename;
            fileInfo.fileId = fileId;
        }
    }
}

void NTFSRecovery::processDataAttribute(const AttributeHeader* attr, const uint8_t* attrData, bool isDeleted, NTFSFileInfo& fileInfo) {
    if (attr->nonResident) {
        const NonResidentAttributeHeader* nonResident =
            reinterpret_cast<const NonResidentAttributeHeader*>(attrData);

        fileInfo.fileSize = nonResident->realSize;

        const uint8_t* runList = attrData + nonResident->dataRunOffset;
        uint64_t currentLCN = 0;

        while (*runList) {
            uint8_t header = *runList++;
            uint8_t lengthSize = header & 0x0F;
            uint8_t offsetSize = (header >> 4) & 0x0F;

            if (lengthSize == 0) break;

            // Read run length
            uint64_t runLength = 0;
            for (int i = 0; i < lengthSize; i++) {
                runLength |= static_cast<uint64_t>(*runList++) << (i * 8);
            }

            // Read run offset (can be negative)
            int64_t runOffset = 0;
            if (offsetSize > 0) {
                for (int i = 0; i < offsetSize; i++) {
                    runOffset |= static_cast<uint64_t>(*runList++) << (i * 8);
                }

                // Sign extend if the highest bit is set
                if (runOffset & (1ULL << ((offsetSize * 8) - 1))) {
                    runOffset |= ~((1LL << (offsetSize * 8)) - 1); // Apply sign extension
                }
            }

            currentLCN += runOffset;
            if (currentLCN > driveInfo.bootSector.totalSectors / driveInfo.bootSector.sectorsPerCluster) {
                break;
            }

            if (isDeleted) {
                fileInfo.cluster = currentLCN;
                fileInfo.runLength = runLength;
                fileInfo.nonResident = true;
            }
        }
    }
    else {
        const ResidentAttributeHeader* resident = reinterpret_cast<const ResidentAttributeHeader*>(attrData);
        const uint8_t* residentData = attrData + resident->contentOffset;

        if (isDeleted) {
            fileInfo.fileSize = resident->contentLength;
            fileInfo.nonResident = false;
            fileInfo.data.assign(residentData, residentData + resident->contentLength);
        }
    }
}



void NTFSRecovery::addToRecoveryList(const NTFSFileInfo& fileInfo) {
    recoveryList.push_back(fileInfo);
}


/* Recovery */
std::vector<NTFSFileInfo> NTFSRecovery::selectFilesToRecover(const std::vector<NTFSFileInfo>& recoveryList) {
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
            std::vector<NTFSFileInfo> selectedFiles;

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

void NTFSRecovery::runLogicalDriveRecovery() {
    scanForDeletedFiles();
    recoverPartition();
}

void NTFSRecovery::recoverPartition() {
    utils.printHeader("File Recovery and Analysis:");
    if (recoveryList.empty()) {
        if (config.recover || config.analyze) std::cerr << "[-] No deleted files found" << std::endl;
        else std::cout << "[!] Recovery or analysis is disabled. Use --recover and/or --analyze to proceed." << std::endl;

        return;
    }

    std::vector<NTFSFileInfo> selectedDeletedFiles;
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

void NTFSRecovery::processFileForRecovery(const NTFSFileInfo& fileInfo) {
    bool isExtensionPredicted = false;

    // Skip files that don't match the target cluster and size (if specified)
    if (fileInfo.fileSize <= 0 || (config.targetCluster && config.targetFileSize && (fileInfo.cluster != config.targetCluster || fileInfo.fileSize != config.targetFileSize))) {
        return;
    }


    fs::path outputPath = utils.getOutputPath(fileInfo.fileName, config.outputFolder);
    uint64_t expectedSize = fileInfo.fileSize;

    NTFSRecoveryStatus status = {};


    status.expectedClusters = (expectedSize + driveInfo.bytesPerCluster - 1) / driveInfo.bytesPerCluster;

    std::wcout << "[*] Current file: " << outputPath.filename() << " (" << expectedSize << " bytes)" << std::endl;


    if (fileInfo.nonResident) {
        std::vector<uint64_t> clusterChain = validateClusterChain(status, fileInfo, expectedSize, outputPath, isExtensionPredicted);
        if (config.recover) {
            recoverNonResidentFile(clusterChain, status, outputPath, expectedSize);
        }
    }
    else {
        if (config.recover) {
            recoverResidentFile(fileInfo, outputPath);
        }
    }
    utils.printItemDivider();
}

void NTFSRecovery::recoverResidentFile(const NTFSFileInfo& fileInfo, const fs::path& outputPath) {
    std::cout << "[*] Recovering file..." << std::endl;
    std::ofstream outputFile(outputPath, std::ios::binary);
    if (!outputFile) {
        throw std::runtime_error("[-] Failed to create output file.");
    }
    outputFile.write(reinterpret_cast<const char*>(fileInfo.data.data()), fileInfo.data.size());
    outputFile.close();
    showRecoveryResult(outputPath);
}

std::vector<uint64_t> NTFSRecovery::validateClusterChain(NTFSRecoveryStatus& status, const NTFSFileInfo& fileInfo, uint64_t expectedSize, const fs::path& outputPath, bool isExtensionPredicted) {
    if (config.analyze) std::cout << "[!] Corruption analysis is not yet implemented for NTFS volumes." << std::endl;
    std::set<uint64_t> usedClusters;
    std::vector<uint64_t> clusterChain;

    uint64_t startCluster = fileInfo.cluster;
    uint64_t length = fileInfo.runLength;
    for (uint64_t i = 0; i < length; ++i) {
        uint64_t currentCluster = startCluster + i;

        // Avoid duplicates
        if (usedClusters.insert(currentCluster).second) {
            clusterChain.push_back(currentCluster);
        }
    }
    return clusterChain;
}

void NTFSRecovery::recoverNonResidentFile(const std::vector<uint64_t>& clusterChain, NTFSRecoveryStatus& status, const fs::path& outputPath, const uint64_t expectedSize) {
    std::cout << "[*] Recovering file..." << std::endl;
    std::ofstream outputFile(outputPath, std::ios::binary);
    if (!outputFile) {
        throw std::runtime_error("[-] Failed to create output file.");
    }
    // Recovery
    for (uint64_t cluster : clusterChain) {
        uint64_t sector = clusterToSector(cluster);

        for (uint64_t i = 0; i < driveInfo.bootSector.sectorsPerCluster; ++i) {
            std::vector<uint8_t> sectorBuffer(driveInfo.bootSector.bytesPerSector);
            if (!readSector(sector + i, sectorBuffer.data(), driveInfo.bootSector.bytesPerSector)) {
                continue;
            }

            uint64_t bytesToWrite = (std::min)(
                static_cast<uint64_t>(driveInfo.bootSector.bytesPerSector),
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
    std::cout << "\n";
    showRecoveryResult(outputPath);
}

/* Recovery and analysis results */
void NTFSRecovery::showRecoveryResult(const fs::path& outputPath) const {
    if (fs::exists(fs::absolute(outputPath))) {
        std::wcout << "  [+] File saved to " << fs::absolute(outputPath) << L"\n";
    }
    else std::wcout << "  [-] Failed to save file" << std::endl;
}
// implement analysis

/* Public */
/* Entry point */
void NTFSRecovery::startRecovery() {

    if (this->driveType == DriveType::LOGICAL_TYPE) runLogicalDriveRecovery();
    else {
        throw std::runtime_error("Unknown drive type.");
    }

}
