#ifndef __FAT12_H
#define __FAT12_H

#include "fatfs.h"

/**
 * Maximum file name length.
 */
#define NAME_LENGTH                 8

/**
 * Maximum file extension length.
 */
#define EXTENSION_LENGTH            3

/**
 * Maximum volume label length.
 */
#define LABEL_LENGTH                11

/**
 * 3.5in, 2 sided, 80 tracks per side, 18 or 36 sectors per track.
 * Total capacity: 1440K or 2880K.
 */
#define MEDIA_TYPE_1440K            0xF0

/**
 * Fixed disk, i.e. non-removable, such as a hard disk.
 */
#define MEDIA_TYPE_FIXED            0xF8

/**
 * Boot sector magic number.
 */
#define BOOT_ID                     0xAA55

/**
 * Boot sector size.
 */
#define BOOT_SECTOR_SIZE            512

#define CLUSTER_FREE                0x0FF
#define CLUSTER_RESERVED            0x001
#define CLUSTER_FIRST               0x002
#define CLUSTER_LAST                0xFEF
#define CLUSTER_BAD                 0xFF7
#define CLUSTER_END                 0xFFF

#define OEM_NAME                    "fatfs   "
#define FS_TYPE                     "FAT12   "
#define LABEL                       "NO NAME    "

/**
 * FAT12 BIOS Parameter Block
 * Contains disk and volume information.
 */
typedef struct __attribute__ ((packed)) _BiosParamBlock
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
} BiosParamBlock;

/**
 * FAT12 boot sector.
 * Contains the initial boot code and volume information.
 */
typedef struct _BootSector
{
    char JumpCode[3];
    char OemName[NAME_LENGTH];
    BiosParamBlock BiosParams;
    char BootCode[448];
    uint16_t Signature;
} BootSector;

typedef struct _FatDate
{
    uint16_t Day       : 5;     // 1-31
    uint16_t Month     : 4;     // 1-12
    uint16_t Year      : 7;     // 0-127 (0 = 1980)
} FatDate;

typedef struct _FatTime
{
    uint16_t Seconds   : 5;     // 0-29 (secs/2)
    uint16_t Minutes   : 6;     // 0-59
    uint16_t Hours     : 5;     // 0-23
} FatTime;

typedef enum __attribute__ ((packed)) _FileAttrs
{
    ATTR_READONLY   = 1 << 0,
    ATTR_HIDDEN     = 1 << 1,
    ATTR_SYSTEM     = 1 << 2,
    ATTR_LABEL      = 1 << 3,
    ATTR_DIRECTORY  = 1 << 4,
    ATTR_ARCHIVE    = 1 << 5,
    ATTR_DEVICE     = 1 << 6,
}  FileAttrs;

typedef struct _DirectoryEntry
{
    char Name[NAME_LENGTH];
    char Extension[EXTENSION_LENGTH];
    FileAttrs Attributes;
    uint8_t _Reserved1;     // varies by system, do not use
    uint8_t _Reserved2;     // TODO: fine creation time
    FatTime CreationTime;
    FatDate CreationDate;
    FatDate LastAccessDate;
    uint16_t _Reserved3;    // TODO: access rights bitmap
    FatTime ModifiedTime;
    FatDate ModifiedDate;
    uint16_t FirstCluster;
    uint32_t FileSize;
} DirectoryEntry;

void InitBPB(BiosParamBlock *bpb);
void InitBootSector(BootSector *bootsect);

void GetLabel(char *dst, const char *src);
void GetName(char *dst, const char *src);
void GetExt(char *dst, const char *src);

static_assert(sizeof(BiosParamBlock) == 51, "Bad BiosParamBlock size!");
static_assert(sizeof(BootSector) == 512, "Bad BootSector size!");
static_assert(sizeof(FatDate) == 2, "Bad FatDate size!");
static_assert(sizeof(FatTime) == 2, "Bad FatTime size!");
static_assert(sizeof(DirectoryEntry) == 32, "Bad DirectoryEntry size!");

static_assert(offsetof(BiosParamBlock, SectorSize) == 0x00, "Bad SectorSize offset!");
static_assert(offsetof(BiosParamBlock, SectorsPerCluster) == 0x02, "Bad SectorsPerCluster offset!");
static_assert(offsetof(BiosParamBlock, ReservedSectorCount) == 0x03, "Bad ReservedSectorCount offset!");
static_assert(offsetof(BiosParamBlock, TableCount) == 0x05, "Bad TableCount offset!");
static_assert(offsetof(BiosParamBlock, MaxRootDirEntryCount) == 0x06, "Bad MaxRootDirEntryCount offset!");
static_assert(offsetof(BiosParamBlock, SectorCount) == 0x08, "Bad SectorCount offset!");
static_assert(offsetof(BiosParamBlock, MediaType) == 0x0A, "Bad MediaType offset!");
static_assert(offsetof(BiosParamBlock, SectorsPerTable) == 0x0B, "Bad SectorsPerTable offset!");
static_assert(offsetof(BiosParamBlock, SectorsPerTrack) == 0x0D, "Bad SectorsPerTrack offset!");
static_assert(offsetof(BiosParamBlock, HeadCount) == 0x0F, "Bad HeadCount offset!");
static_assert(offsetof(BiosParamBlock, HiddenSectorCount) == 0x11, "Bad HiddenSectorCount offset!");
static_assert(offsetof(BiosParamBlock, LargeSectorCount) == 0x15, "Bad LargeSectorCount offset!");
static_assert(offsetof(BiosParamBlock, DriveNumber) == 0x19, "Bad DriveNumber offset!");
static_assert(offsetof(BiosParamBlock, _Reserved) == 0x1A, "Bad _Reserved offset!");
static_assert(offsetof(BiosParamBlock, ExtendedBootSignature) == 0x1B, "Bad ExtendedBootSignature offset!");
static_assert(offsetof(BiosParamBlock, VolumeId) == 0x1C, "Bad VolumeId offset!");
static_assert(offsetof(BiosParamBlock, Label) == 0x20, "Bad Label offset!");
static_assert(offsetof(BiosParamBlock, FileSystemType) == 0x2B, "Bad FileSystemType offset!");

#endif // __FAT12_H
