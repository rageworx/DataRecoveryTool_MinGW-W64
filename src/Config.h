#pragma once
#include <string>
#include <cstdint>

#pragma pack(push, 1)
class Config {
private:
    Config() {}  // Private constructor to prevent direct instantiation
    Config(const Config&) = delete;  // Prevent copy constructor
    Config& operator=(const Config&) = delete;  // Prevent assignment operator
public:
    // Access the unique instance of Config
    static Config& getInstance() {
        static Config instance; 
        return instance;
    }

    std::wstring drivePath = L"";
    std::wstring inputFolder = L""; // not implemented yed
    std::wstring outputFolder = L"Recovered";
    std::wstring logFolder = L"Log";
    std::wstring logFile = L"FileDataLog.txt";
    uint32_t targetCluster = 0; // not implemented yed
    uint32_t targetFileSize = 0; // not implemented yed
    bool createFileDataLog = true;
    bool recover = false;
    bool analyze = false;


};
#pragma pack(pop)