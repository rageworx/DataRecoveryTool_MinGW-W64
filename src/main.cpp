
#include "DriveHandler.h"
#include <iostream>
#include <windows.h>
#include <string>
#include <sstream>
// Helper function to convert string to wstring
std::wstring stringToWstring(const std::string& str) {
    if (str.empty()) {
        return std::wstring();
    }

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Helper function to print usage information
void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " [OPTIONS]\n"
        << "Options:\n"
        << "  -h, --help                          Show this help message\n"
        << "  -d, --drive <drive>                 [REQUIRED] Specify the drive path\n"
        << "  -r, --recover                       [OPTIONAL] Perform file recovery (default: DISABLED)\n"
        << "  -a, --analyze                       [OPTIONAL] Analyze clusters for corruption (time-consuming) (default: DISABLED)\n"
        << "  -l, --no-log                        [OPTIONAL] Disable logging found files and their location (default: LOGGING ENABLED)\n"
        << "  -c, --cluster <number>              [OPTIONAL] Recover a single file by cluster (requires --size and FileDataLog.txt)\n"
        << "  -s, --size <size>                   [OPTIONAL] Target file size (bytes) for single-file recovery (requires --cluster)\n"
        << "  -o, --output-folder <folder>        [OPTIONAL] Specify the output folder (default: Recovered)\n"
        << "  -i, --input-folder <folder>         [OPTIONAL] Specify the folder to recover files from (default: ROOT DIRECTORY)\n";

    std::cerr << "\nExamples:\n"
        << "  1. Logical Drive:\n"
        << "        " << programName << " --drive F: --recover\n"
        << "  2. Physical Drive:\n"
        << "        " << programName << " --drive PhysicalDrive3 --recover\n";

    std::cerr << "\nNotes:\n"
        << "  - Selecting specific files for recovery:\n"
        << "      1. Run the program with the '--recover' argument to interactively choose files to recover.\n"
        << "  - Automating recovery:\n"
        << "      1. Use '--cluster' and '--size' arguments to specify file details directly.\n"
        << "      2. Use '--no-log' argument when automating recovery to avoid creating extra files.\n"
        << "  - Log file format:\n"
        << "      * The `FileDataLog.txt` is in CSV format, facilitating easy automation.\n"
        << "  - File corruption analysis:\n"
        << "      * Use '--analysis' argument to scan recovered file for potential corruption.\n"
        << "  - Supported file systems:\n"
        << "      * Currently, only FAT32 file recovery is supported.\n";

}

// Function to parse command line arguments
Config parseCommandLine(int argc, char* argv[]) {
    Config config = {
        .drivePath = L"F:",
        .inputFolder = L"",
        .outputFolder = L"Recovered",
        .logFolder = L"Log",
        .logFile = L"FileDataLog.txt",
        .targetCluster = 10,
        .targetFileSize = 48,
        .createFileDataLog = true,
        .recover = true,
        .analyze = true,
    };
    
    bool driveSpecified = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        try {
            if (arg == "-d" || arg == "--drive") {
                if (i + 1 < argc) {
                    config.drivePath = stringToWstring(argv[++i]);
                    driveSpecified = true;
                }
                else {
                    throw std::runtime_error("--drive argument is missing");
                }
            }
            else if (arg == "-i" || arg == "--input-folder") {
                if (i + 1 < argc) {
                    config.inputFolder = stringToWstring(argv[++i]);
                }
                else {
                    throw std::runtime_error("--input-folder argument is missing");
                }
            }
            else if (arg == "-o" || arg == "--output-folder") {
                if (i + 1 < argc) {
                    config.outputFolder = stringToWstring(argv[++i]);
                }
                else {
                    throw std::runtime_error("--output-folder argument is missing");
                }
            }
            else if (arg == "-c" || arg == "--cluster") {
                if (i + 1 < argc) {
                    config.targetCluster = static_cast<uint32_t>(std::stoul(argv[++i]));
                }
                else {
                    throw std::runtime_error("--cluster argument is missing");
                }
            }
            else if (arg == "-s" || arg == "--size") {
                if (i + 1 < argc) {
                    config.targetFileSize = static_cast<uint32_t>(std::stoul(argv[++i]));
                }
                else {
                    throw std::runtime_error("--target-filesize argument is missing");
                }
            }
            else if (arg == "-l" || arg == "--no-log") {
                config.createFileDataLog = false;
            }
            else if (arg == "-r" || arg == "--recover") {
                config.recover = true;
            }
            else if (arg == "-a" || arg == "--analyze") {
                config.analyze = true;
            }
            else if (arg == "-h" || arg == "--help") {
                printUsage(argv[0]);
                exit(0);
            }
            else {
                throw std::runtime_error("Unknown argument: " + arg);
            }
        }
        
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            printUsage(argv[0]);
            exit(1);
        }
    }

    if (config.drivePath.empty()) {
        throw std::runtime_error("--drive argument is missing");
    }

    if ((config.targetCluster && !config.targetFileSize) || (!config.targetCluster && config.targetFileSize)) {
        char userInput;
        std::cout << "In order to recover a single file, both '--cluster' and '--size' must be specified." << std::endl;
        std::cout << "Do you want to recover all files? [Y/n]: ";
        std::cin >> userInput;
        userInput = toupper(userInput);
        if (userInput == 'Y') {
            config.targetCluster = 0;
            config.targetFileSize = 0;
        }
        else {
            printUsage(argv[0]);
            exit(1);
        }
    }

    return config;
}


void printConfig(const Config& config) {
    std::cout << std::string(60, '_') << "\n\n";
    std::cout << "Configuration Details:" << std::endl;
    std::cout << std::string(60, '_') << "\n";
    std::cout << "\n";
    std::wcout
        << L"  Drive Path             | " << config.drivePath << L"\n"
        << L"  Input Folder           | " << (!config.inputFolder.empty() ? config.inputFolder : L"All folders") << L"\n"
        << L"  Output Folder          | " << (!config.outputFolder.empty() ? config.outputFolder : L"Recovered") << L"\n"
        << L"  Target Cluster         | " << (config.targetCluster ? std::to_wstring(config.targetCluster) : L"Not specified") << L"\n"
        << L"  Target File Size       | " << (config.targetFileSize ? std::to_wstring(config.targetFileSize) : L"Not specified") << L"\n"
        << L"  Create File Data Log   | " << (config.createFileDataLog ? L"Yes" : L"No") << L"\n"
        << L"  Recover Files          | " << (config.recover ? L"Yes" : L"No") << L"\n"
        << L"  Analyze Files          | " << (config.analyze ? "Yes" : "No") << L"\n";
    std::cout << std::string(60, '_') << "\n\n";
}

int main(int argc, char* argv[]) {
    try {
        Config config = parseCommandLine(argc, argv);
        printConfig(config);
        DriveHandler driveHandler(config);
        driveHandler.recoverDrive();
    }
    catch (const std::exception& e) {
        std::cerr << "[-] Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}