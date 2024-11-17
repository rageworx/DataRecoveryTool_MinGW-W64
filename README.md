# File Recovery Tool

This tool is designed for recovering deleted files from FAT32 and exFAT file systems. It allows recovery on specified drives using the **FAT32**, **exFAT** and **NTFS** format. Future updates will expand compatibility to EXT4, with added Linux support for both FAT32 and exFAT.

**Important:** If you need to recover deleted files, avoid downloading or installing this tool on the drive you're recovering from to prevent overwriting lost data.

## Current support
- **NTFS**
- **FAT32**
- **exFAT**


## Features
- **Corruption Detection:** Analyze each file for potential corruption, helping ensure data integrity in recovered files.
- **Automated File Logging:** Generate a .txt log file listing deleted files.
- **File Type Prediction:** Attempt to predict file extensions for corrupted files, simplifying the identification of unknown file types during recovery (for FAT32).
- **Automatic Filesystem Detection:**  Detect the filesystem type (FAT32, exFAT, ...).
- **Read-Only Access:** Drives are accessed as read-only to prevent any accidental data changes or damage.

## Usage

```
Usage: <program_name> [OPTIONS]

Options:
  -h, --help                          Show this help message
  -d, --drive <drive>                 [REQUIRED] Specify the drive path (e.g., F:)
  -r, --recover                       [OPTIONAL] Perform file recovery
  -a, --analyze                       [OPTIONAL] Analyze files for corruption (time-consuming)
  -l, --no-log                        [OPTIONAL] Disable logging found files and their location
```
### Behavior

* When the `--recover` and/or `--analyze` argument is specified and deleted files are found, you will be prompted to choose specific or all files to process.
* When only `--drive` argument is specified, the program will only search for the deleted files, without recovering them.

## Examples

1. **Recover files:**
    ```
    <program_name> --drive F: --recover
    ```
    - You will be prompted to choose which files to recover
2. **Recover and analyze deleted files:**
    ```
    <program_name> --drive F: --recover --analyze
    ```
    - Corruption analysis is not yet implemented for NTFS volumes

## Getting Started

### Option 1: Use Precompiled Executable
1. Download the executable file from the `bin` directory of the repository.
2. Open the Command Prompt:
    - Press `Win + R`, type `cmd`, and press Enter.
3. Navigate to the directory containing the executable:
    ```
    cd path\to\bin
    ```
    - Replace `path\to\bin` with the actual path to the folder containing the executable.
4. Run the executable from the Command Prompt:
    ```
    <executable_name>.exe [OPTIONS]
    ```
    - Replace `<executable_name>` with the name of the executable file (eg., `DataRecoveryTool_x64`)
    - Use the options from the **Usage** section.
### Option 2: Compile the Program
1. Clone the repository:
    ```
    git clone https://github.com/deityyGH/DataRecoveryTool.git
    cd DataRecoveryTool
    ```
2. Open the project in Visual Studio:
    - Open the `.sln` file in the root directory.
3. Set the build configuration to Release for optimal performance.
4. Set the project to use the C++20 standard or higher:
    - Right-click on the project in the **Solution Explorer**, select **Properties**
    - Under **Configuration Properties > General**, set **C++ Language Standard** to **ISO C++20 Standard (/std ++20)** or a higher standard if available.
5. Compile the project:
    - Click **Build > Build Solution** in the Visual Studio top menu.
6. Navigate to the output directory (e.g., ./bin) to find the executable.
7. Run the program from the command line as described in **Option 1**.



## Reporting Issues
-  If you encounter any bugs, please open an issue. I'll do my best to assist or update the code as needed.

## Planned Updates
- Support for EXT4 file system and the older FAT16, FAT8 and EXT3, EXT2.
- Linux compatibility.
- GUI
- Extract data from corrupted files

## License

This project is licensed under the MIT License - see the [LICENSE](https://github.com/deityyGH/DataRecoveryTool_Dev/blob/main/LICENSE) file for details. (Translation: Do whatever you want, just don't blame me if something breaks)
