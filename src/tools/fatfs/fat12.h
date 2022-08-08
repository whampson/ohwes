#ifndef __FAT12_H
#define __FAT12_H

#include "fatfs.h"

#define NAME_LENGTH                 8       // Name capacity,
#define EXT_LENGTH                  3
#define LABEL_LENGTH                11

#define MEDIA_TYPE_1440K            0xF0    // Standard 3.5" floppy disk
#define MEDIA_TYPE_FIXED            0xF8    // Hard disk

#define BOOT_SECTOR_ID              0xAA5
#define BOOT_SECTOR_SIZE            512

#define OEM_NAME                    "fatfs   "
#define DEFAULT_FS_TYPE             "FAT12   "
#define DEFAULT_LABEL               "NO NAME    "

#define CLUSTER_FREE                0x000   // Free data cluster.
#define CLUSTER_RESERVED            0x001   // Reserved, do not use.
#define CLUSTER_FIRST               0x002   // First valid data cluster.
#define CLUSTER_LAST                0xFEF   // Last valid data cluster.
#define CLUSTER_BAD                 0xFF7   // Bad cluster marker.
#define CLUSTER_END                 0xFFF   // End-of-chain marker.

#define IsClusterValid(c)           ((c) >= CLUSTER_FIRST && (c) <= CLUSTER_LAST)

#define MAX_PATH                    512     // completely arbitrary
#define MAX_DATE                    19      // "September 31, 1990"
#define MAX_TIME                    12      // "12:34:56 PM"
#define MAX_SHORTNAME               NAME_LENGTH + EXT_LENGTH + 2

// Year zero.
#define YEAR_BASE                   1980

// Number of long file name characters per entry.
#define LFN_CAPACITY                13

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
    ATTR_LFN        = ATTR_LABEL|ATTR_SYSTEM|ATTR_HIDDEN|ATTR_READONLY
}  FileAttrs;

typedef union _DirEntry
{
    struct
    {
        char Name[NAME_LENGTH];
        char Extension[EXT_LENGTH];
        FileAttrs Attributes;
        uint8_t _Reserved1;     // varies by system, do not use
        uint8_t _Reserved2;     // fine creation time, 10ms increments, 0-199
        FatTime CreationTime;
        FatDate CreationDate;
        FatDate LastAccessDate;
        uint16_t _Reserved3;    // TODO: access rights bitmap?
        FatTime ModifiedTime;
        FatDate ModifiedDate;
        uint16_t FirstCluster;
        uint32_t FileSize;
    };

    struct __attribute__ ((packed))
    {
        char Sequence   : 5;
        char _Reserved0 : 1;
        char FirstEntry : 1;
        char _Reserved1 : 1;
        wchar_t NameChunk1[5];
        FileAttrs Attributes;
        char _Reserved2;
        char Checksum;
        wchar_t NameChunk2[6];
        uint16_t _Reserved3;
        wchar_t NameChunk3[2];
    } LFN;
} DirEntry;


void InitBPB(BiosParamBlock *bpb);
void InitBootSector(BootSector *bootsect);

void GetLabel(char dst[LABEL_LENGTH+1], const char *src);
void GetName(char dst[NAME_LENGTH+1], const char *src);
void GetExt(char dst[EXT_LENGTH+1], const char *src);

void GetShortName(char dst[MAX_SHORTNAME], const DirEntry *file);
char GetShortNameChecksum(const DirEntry *file);

// NOTE: the DirEntry provided must exist within a table of DirEntries
bool ReadLongName(wchar_t dst[MAX_PATH], char *cksum, const DirEntry **entry);

void GetDate(char dst[MAX_DATE], const FatDate *date);
void GetTime(char dst[MAX_TIME], const FatTime *time);

static inline bool IsReadOnly(const DirEntry *e)
{
    return (IsFlagSet(e->Attributes, ATTR_READONLY)
        && !IsFlagSet(e->Attributes, ATTR_LFN));
}

static inline bool IsHidden(const DirEntry *e)
{
    return (IsFlagSet(e->Attributes, ATTR_HIDDEN)
        && !IsFlagSet(e->Attributes, ATTR_LFN));
}

static inline bool IsSystemFile(const DirEntry *e)
{
    return (IsFlagSet(e->Attributes, ATTR_SYSTEM)
        && !IsFlagSet(e->Attributes, ATTR_LFN));
}

static inline bool IsVolumeLabel(const DirEntry *e)
{
    return (IsFlagSet(e->Attributes, ATTR_LABEL)
        && !IsFlagSet(e->Attributes, ATTR_LFN));
}

static inline bool IsDirectory(const DirEntry *e)
{
    return IsFlagSet(e->Attributes, ATTR_DIRECTORY);
}

static inline bool IsDeviceFile(const DirEntry *e)
{
    // I'm not sure if this is even a thing
    return IsFlagSet(e->Attributes, ATTR_DEVICE);
}

static inline bool IsLongFileName(const DirEntry *e)
{
    return IsFlagSet(e->Attributes, ATTR_LFN)
        && e->FirstCluster == 0;
}

static inline bool IsDeleted(const DirEntry *e)
{
    unsigned char c = e->Name[0];
    return c == 0x05 || c == 0xE5;
}

static inline bool IsFree(const DirEntry *e)
{
    return IsDeleted(e) || e->Name[0] == 0x00;
}

static inline bool IsFile(const DirEntry *e)
{
    return !IsFree(e)
        && !IsLongFileName(e)
        && !IsVolumeLabel(e);
}

static inline bool IsRoot(const DirEntry *e)
{
    return IsDirectory(e) && e->FirstCluster == 0;
}

static inline bool IsCurrentDirectory(const DirEntry *e)
{
    return e->Name[0] == '.'
        && e->Name[1] == ' ';
}

static inline bool IsParentDirectory(const DirEntry *e)
{
    return e->Name[0] == '.'
        && e->Name[1] == '.'
        && e->Name[2] == ' ';
}

static_assert(sizeof(BiosParamBlock) == 51, "Bad BiosParamBlock size!");
static_assert(sizeof(BootSector) == 512, "Bad BootSector size!");
static_assert(sizeof(FatDate) == 2, "Bad FatDate size!");
static_assert(sizeof(FatTime) == 2, "Bad FatTime size!");
static_assert(sizeof(DirEntry) == 32, "Bad DirEntry size!");

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
