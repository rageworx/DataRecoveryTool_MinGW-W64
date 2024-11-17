#pragma once
enum class DriveType {
    UNKNOWN_TYPE,
    LOGICAL_TYPE,
    PHYSICAL_TYPE
};

enum class PartitionType {
    UNKNOWN_TYPE,
    MBR_TYPE,
    GPT_TYPE
};

enum class FilesystemType {
    UNKNOWN_TYPE,
    FAT32_TYPE,
    NTFS_TYPE,
    EXFAT_TYPE,
    EXT4_TYPE
};