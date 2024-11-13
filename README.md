# File Recovery Tool

This tool is designed for recovering deleted files from FAT32 and exFAT file systems. It allows recovery on specified drives using the FAT32 or exFAT format. Future updates will expand compatibility to NTFS and EXT4, with added Linux support for both FAT32 and exFAT.

**Important:** If you need to recover deleted files, avoid downloading or installing this tool on the drive you're recovering from to prevent overwriting lost data.

## Current support
- **FAT32**
- **exFAT**


## Features
- **Selective File Recovery:** Recover deleted files on FAT32 drives—whether logical or physical. Select specific files or opt for bulk recovery.
- **Corruption Detection:** Analyze each file for potential corruption, helping ensure data integrity in recovered files.
- **Automated File Logging:** Generate a .txt log file listing deleted file locations.
- **File Type Prediction:** Attempt to predict file extensions for corrupted files, simplifying the identification of unknown file types during recovery.
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

## Reporting Issues
-  If you encounter any bugs, please open an issue. I'll do my best to assist or update the code as needed.

## Planned Updates
- Support for NTFS, and EXT4 file systems.
- Linux compatibility.
- GUI

## License

This project is licensed under the MIT License - see the [LICENSE](https://github.com/deityyGH/DataRecoveryTool_Dev/blob/main/LICENSE) file for details. (Translation: Do whatever you want, just don't blame me if something breaks)
