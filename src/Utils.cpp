#include "Utils.h"
#include <iostream>
#include <Windows.h>

Utils::Utils() : IConfigurable() {}
Utils::~Utils() {
    closeLogFile();
}

void Utils::ensureOutputDirectory() const {
    fs::path outputPath = fs::path(config.outputFolder) / fs::path(config.logFolder);
    try {
        if (!fs::exists(outputPath)) {
            fs::create_directories(outputPath);
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        throw std::runtime_error("Failed to create output directory");
    }
}

fs::path Utils::getOutputPath(const std::wstring& fullName, const std::wstring& folder) const {
    //std::wstring fullName = fileName + L"." + extension;
    fs::path outputPath = fs::path(folder) / fullName;

    size_t dotPos = fullName.find_last_of(L".");
    if (dotPos != std::wstring::npos && dotPos != 0) {
        std::wstring fileName = fullName.substr(0, dotPos);
        std::wstring extension = fullName.substr(dotPos + 1);

        int counter = 1;
        while (fs::exists(outputPath)) {
            std::wstring newName = fileName + L"_" + std::to_wstring(counter);
            if (!extension.empty() && extension != L"") {
                newName += L"." + extension;
            }
            outputPath = fs::path(folder) / newName;
            counter++;
        }
    }
    return outputPath;
}

void Utils::showProgress(uint64_t currentValue, uint64_t maxValue) const {
    float progress = static_cast<float>(currentValue) / maxValue * 100;
    std::cout << "\r[*] Progress: " << std::setw(5) << std::fixed << std::setprecision(2)
        << progress << "%" << std::flush;
}

bool Utils::openLogFile() {
    if (config.createFileDataLog && !logFile.is_open()) {
        fs::path logFolder = fs::path(config.outputFolder) / fs::path(config.logFolder);
        fs::path logFilePath = getOutputPath(config.logFile, logFolder);
        logFile.open(logFilePath, std::ios::app);
    }
    return logFile.is_open();
}
void Utils::logFileInfo(const uint16_t fileId, const std::wstring& fileName, const uint64_t fileSize) {
    std::wcout << "[+] #" << fileId << " Found file \"" << fileName << "\"" << " (" << fileSize << " bytes)" << std::endl;
    if (config.createFileDataLog) {
        writeToLogFile(fileId, fileName, fileSize);
    }
}
void Utils::writeToLogFile(const uint16_t fileId, const std::wstring& fileName, const uint64_t fileSize) {
    if (logFile) {
        std::wstringstream ss;
        ss << L"#" << fileId << L" Filename: \"" << fileName << L"\" (" << fileSize << L" bytes)" << "\n";
        logFile << ss.str();
    }
}
void Utils::closeLogFile() {
    if (logFile.is_open()) {
        logFile.close();
    }
}
bool Utils::confirmProceedWithoutLogFile() const {

    std::wcerr << L"[!] Couldn't open log file." << std::endl;
    std::cerr << "[!] Do you want to proceed restoring all the files? (Recovery will not be affected) [Y/n]: ";

    std::string userResponse = "";

    do {
        //std::cout << "Press ENTER to exit" << std::endl;
        std::getline(std::cin, userResponse);
        std::transform(userResponse.begin(), userResponse.end(), userResponse.begin(),
            [](unsigned char c) { return std::toupper(c); });
        if (userResponse == "Y" || userResponse == "") return true;
        else if (userResponse == "N") {
            return false;
        }
        std::cerr << "Incorrect option." << std::endl;
        std::cerr << "[!] Do you want to proceed restoring all the files? (Recovery will not be affected) [Y/n]: ";
    } while (userResponse.length() != 0);
    return false;
}



void Utils::printHeader(const std::string& stage, char borderChar, int width) const {
    std::cout << stage << std::endl;
    std::cout << std::string(width, borderChar) << "\n\n";
}
void Utils::printFooter(char dividerChar, int width) const {
    std::cout << std::string(width, dividerChar) << "\n\n";
}
void Utils::printItemDivider(char dividerChar, int width) const {
    std::cout << std::string(width, dividerChar) << "\n";
}



