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

//enum class FilesystemType {
//    UNKNOWN_TYPE = 0x00,
//    FAT32_TYPE = 0x0C,
//    NTFS_TYPE = 0x07,
//    EXFAT_TYPE = 0x07,
//    EXT4_TYPE = 0x83
//};

enum class FilesystemType {
    UNKNOWN_TYPE,
    FAT32_TYPE,
    NTFS_TYPE,
    EXFAT_TYPE,
    EXT4_TYPE
};