#pragma once

#include <cstdio>
#include <cinttypes>

#define NAME_LENGTH         8
#define EXT_LENGTH          3
#define LABEL_LENGTH        11
#define BOOT_SIGNATURE      0xAA55

/**
 * FAT12 BIOS Parameter Block
 * Contains disk and volume information.
 */
#pragma pack(1)
struct BiosParameterBlock
{
    uint16_t SectorSize;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectorCount;
    uint8_t TableCount;
    uint16_t MaxRootDirEntryCount;
    uint16_t SectorCount;
    uint8_t MediaType;
    uint16_t SectorsPerTable;

    uint16_t SectorsPerTrack;
    uint16_t HeadCount;
    uint32_t HiddenSectorCount;
    uint32_t LargeSectorCount;

    uint8_t DriveNumber;
    uint8_t _Reserved;
    uint8_t ExtendedBootSignature;
    uint32_t VolumeId;
    char Label[LABEL_LENGTH];
    char FileSystemType[NAME_LENGTH];
};

static_assert(sizeof(BiosParameterBlock) == 51);
static_assert(offsetof(BiosParameterBlock, SectorSize) == 0x00);
static_assert(offsetof(BiosParameterBlock, SectorsPerCluster) == 0x02);
static_assert(offsetof(BiosParameterBlock, ReservedSectorCount) == 0x03);
static_assert(offsetof(BiosParameterBlock, TableCount) == 0x05);
static_assert(offsetof(BiosParameterBlock, MaxRootDirEntryCount) == 0x06);
static_assert(offsetof(BiosParameterBlock, SectorCount) == 0x08);
static_assert(offsetof(BiosParameterBlock, MediaType) == 0x0A);
static_assert(offsetof(BiosParameterBlock, SectorsPerTable) == 0x0B);
static_assert(offsetof(BiosParameterBlock, SectorsPerTrack) == 0x0D);
static_assert(offsetof(BiosParameterBlock, HeadCount) == 0x0F);
static_assert(offsetof(BiosParameterBlock, HiddenSectorCount) == 0x11);
static_assert(offsetof(BiosParameterBlock, LargeSectorCount) == 0x15);
static_assert(offsetof(BiosParameterBlock, DriveNumber) == 0x19);
static_assert(offsetof(BiosParameterBlock, _Reserved) == 0x1A);
static_assert(offsetof(BiosParameterBlock, ExtendedBootSignature) == 0x1B);
static_assert(offsetof(BiosParameterBlock, VolumeId) == 0x1C);
static_assert(offsetof(BiosParameterBlock, Label) == 0x20);
static_assert(offsetof(BiosParameterBlock, FileSystemType) == 0x2B);

/**
 * FAT12 bootsector.
 * Contains the initial boot code and volume information.
 */
struct BootSector
{
    char JumpCode[3];
    char OemName[NAME_LENGTH];
    BiosParameterBlock BiosParams;
    char BootCode[448];
    uint16_t BootSignature;
};

static_assert(sizeof(BootSector) == 512);

// struct FatDate
// {
//     uint16_t Day       : 5;     // 1-31
//     uint16_t Month     : 4;     // 1-12
//     uint16_t Year      : 7;     // 0-127 (0 = 1980)
// };

// struct FatTime
// {
//     uint16_t Seconds   : 5;     // 0-29 (secs/2)
//     uint16_t Minutes   : 6;     // 0-59
//     uint16_t Hours     : 5;     // 0-23
// };

// struct DirectoryEntry
// {
//     char Name[8];
//     char Extension[3];
//     FileAttrs Attributes;
//     uint8_t _Reserved1;     // varies by system, do not use
//     uint8_t _Reserved2;     // TODO: fine creation time
//     FatTime CreationTime;
//     FatDate CreationDate;
//     FatDate LastAccessDate;
//     uint16_t _Reserved3;    // TODO: access rights bitmap
//     FatTime ModifiedTime;
//     FatDate ModifiedDate;
//     uint16_t FirstCluster;
//     uint32_t FileSize;
// };

