#pragma once
#include "IConfigurable.h"
#include <filesystem>
#include <cstdint>
#include <string>
#include <fstream>

namespace fs = std::filesystem;



class Utils : public IConfigurable{
private:
    std::wofstream logFile;
public:
    Utils();
    ~Utils();
    
    // Creates output folder and log folder
    void ensureOutputDirectory() const;
    fs::path getOutputPath(const std::wstring& fullName, const std::wstring& folder) const;
    void showProgress(uint64_t currentValue, uint64_t maxValue) const;

    /*=============== File Log Operations ===============*/
    bool openLogFile();
    void logFileInfo(const uint16_t fileId, const std::wstring& fileName, const uint64_t fileSize);
    void writeToLogFile(const uint16_t fileId, const std::wstring& fileName, const uint64_t fileSize);
    bool confirmProceedWithoutLogFile() const;
    void closeLogFile();

    /*=============== Print terminal dividers for better readability ===============*/
    void printHeader(const std::string& stage, char borderChar = '_', int width = 60) const;
    void printFooter(char dividerChar = '_', int width = 60) const;
    void printItemDivider(char dividerChar = '-', int width = 60) const;
};