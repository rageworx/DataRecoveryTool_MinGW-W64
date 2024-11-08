# File Recovery Tool

This is a data recovery tool for deleted files on FAT32 file systems. Currently, the program supports recovering files on specified drives (logical or physical) in FAT32 format. Future updates will add support for NTFS and EXT4 file systems, and will include Linux support for FAT32 and exFAT.

If you are looking to recover deleted files, don't download this program to the drive you want to recover files from, to prevent overwriting the deleted files.

## Current support
- **FAT32**
- **exFAT**


## Features
- **Selective File Recovery:** Recover deleted files on FAT32 drives—whether logical or physical. Select specific files or opt for bulk recovery.
- **Corruption Detection:** Analyze each file for potential corruption, helping ensure data integrity in recovered files.
- **Automated File Logging:** Generate a .txt log file listing deleted file locations.
- **File Type Prediction:** Attempt to predict file extensions for corrupted files, simplifying the identification of unknown file types during recovery.
- **Automatic Filesystem Detection:**  Detect the filesystem type (FAT32, exFAT, ...).

## Usage

```
Usage: <program_name> [OPTIONS]

Options:
  -h, --help                          Show this help message
  -d, --drive <drive>                 [REQUIRED] Specify the drive path (e.g., F:)
  -r, --recover                       [OPTIONAL] Perform file recovery
  -a, --analyze                       [OPTIONAL] Analyze files for corruption (time-consuming)
  -l, --no-log                        [OPTIONAL] Disable logging found files and their location (default: LOGGING ENABLED)
```
* When the `--recover` and/or `--analyze` argument is specified and deleted files are found, you will be prompted to choose specific or all files to process.
* When only `--drive` argument is specified, the program will only search for the deleted files, without recovering them.

## Examples

1. **Recover files:**
    ```cmd
    <program_name> --drive F: --recover
    ```
    - You will be prompted to choose which files to recover
2. **Recover and analyze deleted files:**
    ```cmd
    <program_name> --drive F: --recover --analyze
    ```

## Notes
- **Read-Only Access:** Drives are accessed as read-only to prevent any accidental data changes or damage.
- **Reporting Issues:** If you encounter any bugs, please open an issue. I'll do my best to assist or update the code as needed.

## Planned Updates
- Support for NTFS, and EXT4 file systems.
- Linux compatibility.