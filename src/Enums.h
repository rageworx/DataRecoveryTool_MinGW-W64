#pragma once
enum class DriveType {
    UNKNOWN_DRIVE = -1,
    LOGICAL_DRIVE = 1,
    PHYSICAL_DRIVE = 2
};

enum class PartitionScheme {
    UNKNOWN_SCHEME = -1,
    MBR_SCHEME = 1,
    GPT_SCHEME = 2
};

enum class FilesystemType {
    UNKNOWN_TYPE = 0x00,
    FAT32_TYPE = 0x0C,
    NTFS_TYPE = 0x07,
    EXFAT_TYPE = 0x07,
    EXT4_TYPE = 0x83
};

