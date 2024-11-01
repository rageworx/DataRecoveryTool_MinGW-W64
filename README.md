# FAT32 File Recovery Tool

This is a data recovery tool for deleted files on FAT32 file systems. Currently, the program supports recovering files on specified drives (logical or physical) in FAT32 format. Future updates will add support for NTFS, exFAT, and EXT4 file systems, and will include Linux support for FAT32 and exFAT.

## Features
- **Selective File Recovery:** Recover deleted files on FAT32 drives—whether logical or physical. Select specific files or opt for bulk recovery.
- **Corruption Detection:** Analyze each file for potential corruption, helping ensure data integrity in recovered files.
- **Targeted File Recovery:** Specify file clusters and sizes to pinpoint individual files, useful for automation and precise recovery.
- **Automated File Logging:** Generate a CSV-formatted .txt log file listing deleted file locations for easy reference and automation.
- **Customizable Input/Output:** Define both the source and destination folders for recovery, allowing organized and efficient file retrieval.
- **File Type Prediction:** Attempt to predict file extensions for corrupted files, simplifying the identification of unknown file types during recovery.

## Usage

```
Usage: <program_name> [OPTIONS]

Options:
  -h, --help                          Show this help message
  -d, --drive <drive>                 [REQUIRED] Specify the drive path (e.g., F: or PhysicalDrive3)
  -r, --recover                       [OPTIONAL] Perform file recovery
  -a, --analyze                       [OPTIONAL] Analyze files for corruption (time-consuming)
  -i, --input-folder <folder>         [OPTIONAL] Specify the folder to recover files from (default: root directory)
  -o, --output-folder <folder>        [OPTIONAL] Specify the output folder (default: Recovered)
  -c, --cluster <number>              [OPTIONAL] Recover a single file by cluster (requires --size), both values can be found in `FileDataLog.txt`
  -s, --size <size>                   [OPTIONAL] Target file size (bytes) for single-file recovery (requires --cluster)
  -l, --no-log                        [OPTIONAL] Disable logging found files and their location (default: LOGGING ENABLED)
```
* When the `--recover` and/or `--analyze` argument is specified and deleted files are found, you will be prompted to choose specific files to process or process all files.

## Examples

### Logical Drive Examples
1. **Recover files:**
    ```cmd
    <program_name> --drive F: --recover
    ```
    - You will be prompted to choose which files to recover
2. **Recover and analyze deleted files:**
    ```cmd
    <program_name> --drive F: --recover --analyze
    ```
    - 
3. **Recover / Analyze a specific file by cluster and size:**
    ```cmd
    <program_name> --drive F: --cluster 9 --size 1721
    ```
    - Cluster and size can be found in `FileDataLog.txt`
    - This single file recovery method is recommended for automation

### Physical Drive Examples
1. **Recover files:**
    ```cmd
    <program_name> --drive PhysicalDrive3 --recover
    ```

## Finding the `PhysicalDrive`

**Using Command Prompt**
```bash
wmic diskdrive list brief
```
**Using PowerShell**
```powershell
Get-WmiObject -Class Win32_DiskDrive | Select-Object DeviceID, Model, Size
```
**Example Output**

| DeviceID           | Model                                |         Size |
| :---               | :----:                               | ---:         |
| \\\\.\\PHYSICALDRIVE3 | Kingston DataTraveler 3.0 USB Device |  31025756160 |

Both commands will display a list of drives. Look for the DeviceID value (e.g., PhysicalDrive0, PhysicalDrive1) to identify the drive number you need. You should omit the `\\.\` part.

## Notes
- **Admin Privileges Required:** When using a Physical Drive, admin privileges are necessary. In most cases, results will be the same as using a Logical Drive.
- **Read-Only Access:** Drives are accessed as read-only to prevent any accidental data changes or damage.
- **Reporting Issues:** If you encounter any bugs, please open an issue. I'll do my best to assist or update the code as needed.

## Planned Updates
- Support for NTFS, exFAT, and EXT4 file systems.
- Linux compatibility