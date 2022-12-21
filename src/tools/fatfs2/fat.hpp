#ifndef FAT_HPP
#define FAT_HPP

#include "fatfs.hpp"

#define NAME_LENGTH         8                       // Name buffer length
#define EXTENSION_LENGTH    3                       // Filename extension buffer length
#define LABEL_LENGTH        11                      // Volume label buffer length

#define MAX_NAME            (NAME_LENGTH+1)         // Name buffer length, including NULL terminator
#define MAX_EXTENSION       (EXTENSION_LENGTH+1)    // Extension buffer length, including NULL terminator
#define MAX_LABEL           (LABEL_LENGTH+1)        // Label buffer length, including NULL terminator
#define MAX_PATH            512                     // File path length; completely arbitrary

#define MEDIATYPE_1440K     0xF0                    // Standard 3.5" floppy disk
#define MEDIATYPE_FIXED     0xF8                    // Hard disk

#define BOOTSIG             0xAA55                  // Boot sector signature
#define BPBSIG_DOS41        0x29                    // DOS 4.1 BIOS Param Block

#define FIRST_CLUSTER       2
#define LAST_CLUSTER_12     0xFF6
#define LAST_CLUSTER_16     0xFFF6
#define MIN_CLUSTER_12      1
#define MAX_CLUSTER_12      (LAST_CLUSTER_12-FIRST_CLUSTER)
#define MIN_CLUSTER_16      (MAX_CLUSTER_12+1)
#define MAX_CLUSTER_16      (LAST_CLUSTER_16-FIRST_CLUSTER)

#define CLUSTER_FREE        0
#define CLUSTER_RESERVED    1
#define CLUSTER_BAD         0xFFF7
#define CLUSTER_EOC         0xFFFF

#define MIN_SECTOR_SIZE     512
#define MAX_SECTOR_SIZE     32768

#define MIN_SEC_PER_CLUST   1
#define MAX_SEC_PER_CLUST   128

static_assert(MAX_CLUSTER_12 == 4084, "Bad max FAT12 cluster size!");
static_assert(MAX_CLUSTER_16 == 65524, "Bad max FAT16 cluster size!");

/**
 * BIOS Parameter Block
 * Contains disk and volume information.
 *
 * This appears to be the MSDOS 4.0 version of the BPB.
 *
 * https://jdebp.uk/FGA/bios-parameter-block.html
 * https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#BPB
 */
struct BiosParamBlock
{
    // DOS 2.0 BPB
    uint16_t SectorSize;                // Size of a sector in bytes
    uint8_t SectorsPerCluster;          // Number of sectors per data cluster
    uint16_t ReservedSectorCount;       // Number of sectors in the reserved area
    uint8_t TableCount;                 // Number of File Allocation Tables on disk
    uint16_t RootDirCapacity;           // Number of entries allowed in the root directory
    uint16_t SectorCount;               // Total number of sectors on disk
    uint8_t MediaType;                  // Physical type identifier
    uint16_t SectorsPerTable;           // Number of sectors per File Allocation Table

    // DOS 3.31 BPB
    uint16_t SectorsPerTrack;           // Number of sectors per physical track on disk
    uint16_t HeadCount;                 // Number of physical heads on disk
    uint32_t HiddenSectorCount;         // Number of hidden sectors, not supported unless disk is partitioned
    uint32_t SectorCountLarge;          // Total number of sectors on disk if 'SectorCount' exceeds 16 bits

    // DOS 4.1 BPB
    uint8_t DriveNumber;                // Disk drive number for BIOS I/O purposes
    uint8_t _Reserved;                  // Reserved; MSDOS uses this for chkdsk
    uint8_t Signature;                  // BPB version signature
    uint32_t VolumeId;                  // Volume serial number
    char Label[LABEL_LENGTH];           // Volume label
    char FsType[NAME_LENGTH];           // File system identifier
} __attribute__((packed));

static_assert(offsetof(BiosParamBlock, SectorSize) == 0x00, "Bad SectorSize offset!");
static_assert(offsetof(BiosParamBlock, SectorsPerCluster) == 0x02, "Bad SectorsPerCluster offset!");
static_assert(offsetof(BiosParamBlock, ReservedSectorCount) == 0x03, "Bad ReservedSectorCount offset!");
static_assert(offsetof(BiosParamBlock, TableCount) == 0x05, "Bad TableCount offset!");
static_assert(offsetof(BiosParamBlock, RootDirCapacity) == 0x06, "Bad RootDirCapacity offset!");
static_assert(offsetof(BiosParamBlock, SectorCount) == 0x08, "Bad SectorCount offset!");
static_assert(offsetof(BiosParamBlock, MediaType) == 0x0A, "Bad MediaType offset!");
static_assert(offsetof(BiosParamBlock, SectorsPerTable) == 0x0B, "Bad SectorsPerTable offset!");
static_assert(offsetof(BiosParamBlock, SectorsPerTrack) == 0x0D, "Bad SectorsPerTrack offset!");
static_assert(offsetof(BiosParamBlock, HeadCount) == 0x0F, "Bad HeadCount offset!");
static_assert(offsetof(BiosParamBlock, HiddenSectorCount) == 0x11, "Bad HiddenSectorCount offset!");
static_assert(offsetof(BiosParamBlock, SectorCountLarge) == 0x15, "Bad SectorCountLarge offset!");
static_assert(offsetof(BiosParamBlock, DriveNumber) == 0x19, "Bad DriveNumber offset!");
static_assert(offsetof(BiosParamBlock, _Reserved) == 0x1A, "Bad _Reserved offset!");
static_assert(offsetof(BiosParamBlock, Signature) == 0x1B, "Bad Signature offset!");
static_assert(offsetof(BiosParamBlock, VolumeId) == 0x1C, "Bad VolumeId offset!");
static_assert(offsetof(BiosParamBlock, Label) == 0x20, "Bad Label offset!");
static_assert(offsetof(BiosParamBlock, FsType) == 0x2B, "Bad FsType offset!");
static_assert(sizeof(BiosParamBlock) == 51, "Bad BiosParamBlock size!");

/**
 * Boot Sector
 * Contains the initial boot code and volume information.
 */
struct BootSector
{
    char JumpCode[3];                   // Small bit of code that jumps to boot loader code
    char OemName[NAME_LENGTH];          // Format utility identifier
    BiosParamBlock BiosParams;          // BIOS Parameter Block
    char BootCode[448];                 // Boot loader code
    uint16_t BootSignature;             // Boot sector signature, indicating whether the disk is bootable
};

static_assert(offsetof(BootSector, JumpCode) == 0x000, "Bad JumpCode offset!");
static_assert(offsetof(BootSector, OemName) == 0x003, "Bad OemName offset!");
static_assert(offsetof(BootSector, BiosParams) == 0x00B, "Bad BiosParams offset!");
static_assert(offsetof(BootSector, BootCode) == 0x03E, "Bad BootCode offset!");
static_assert(offsetof(BootSector, BootSignature) == 0x1FE, "Bad BootSignature offset!");
static_assert(sizeof(BootSector) == 512, "Bad BootSector size!");

/**
 * Date
*/
struct FatDate
{
    uint16_t Day       : 5;             // Day of month; 1-31
    uint16_t Month     : 4;             // Month of year; 1-12
    uint16_t Year      : 7;             // Calendar year; 0-127 (0 = 1980)
};
static_assert(sizeof(FatDate) == 2, "Bad FatDate size!");

/**
 * Time
*/
struct FatTime
{
    uint16_t Seconds   : 5;             // Seconds; 0-29 (secs/2)
    uint16_t Minutes   : 6;             // Minutes; 0-59
    uint16_t Hours     : 5;             // Hours; 0-23
};
static_assert(sizeof(FatTime) == 2, "Bad FatTime size!");

/**
 * File Attributes
*/
enum FileAttrs : char
{
    ATTR_READONLY   = 1 << 0,           // Read-only
    ATTR_HIDDEN     = 1 << 1,           // Hidden
    ATTR_SYSTEM     = 1 << 2,           // System, do not move
    ATTR_LABEL      = 1 << 3,           // Directory volume label
    ATTR_DIRECTORY  = 1 << 4,           // Directory
    ATTR_ARCHIVE    = 1 << 5,           // Dirty bit (used for backup utilities)
    ATTR_DEVICE     = 1 << 6,           // Device file (not found on disk)
    ATTR_LFN        = ATTR_LABEL        // Long File Name (special/hack)
                     |ATTR_SYSTEM
                     |ATTR_HIDDEN
                     |ATTR_READONLY
};
static_assert(sizeof(FileAttrs) == 1, "Bad FileAttrs size!");

/**
 * Directory Entry
 * Contains timestamps, attributes, size, and location information about a file
 * or directory.
*/
struct DirEntry
{
    char Name[NAME_LENGTH];
    char Extension[EXTENSION_LENGTH];
    FileAttrs Attributes;
    uint8_t _Reserved1;     // varies by system, do not use
    uint8_t _Reserved2;     // TODO: fine creation time, 10ms increments, 0-199
    FatTime CreationTime;
    FatDate CreationDate;
    FatDate LastAccessDate;
    uint16_t _Reserved3;    // TODO: access rights bitmap?
    FatTime ModifiedTime;
    FatDate ModifiedDate;
    uint16_t FirstCluster;
    uint32_t FileSize;
};
static_assert(sizeof(DirEntry) == 32, "Bad DirEntry size!");

void InitBiosParamBlock(BiosParamBlock *bpb);
void InitBootSector(BootSector *bootSect, const BiosParamBlock *bpb);

void InitFat12(char *fat, size_t fatSize, char mediaType);
void InitFat16(char *fat, size_t fatSize, char mediaType);

int GetCluster12(const char *fat, size_t fatSize, int index);
int GetCluster16(const char *fat, size_t fatSize, int index);

int SetCluster12(char *fat, size_t fatSize, int index, int value);
int SetCluster16(char *fat, size_t fatSize, int index, int value);

void MakeVolumeLabel(DirEntry *dst, const char *label);

void GetName(char dst[MAX_NAME], const char *src);
void GetExtension(char dst[MAX_EXTENSION], const char *src);
void GetLabel(char dst[MAX_LABEL], const char *src);

void SetName(char dst[NAME_LENGTH], const char *src);
void SetExtension(char dst[EXTENSION_LENGTH], const char *src);
void SetLabel(char dst[LABEL_LENGTH], const char *src);

void SetDate(FatDate *dst, time_t *t);
void SetTime(FatTime *dst, time_t *t);

#define HasAttribute(file,flag) (((file)->Attributes & (flag)) == (flag))

static inline bool IsReadOnly(const DirEntry *e)
{
    return (HasAttribute(e, ATTR_READONLY)
        && !HasAttribute(e, ATTR_LFN));
}

static inline bool IsHidden(const DirEntry *e)
{
    return (HasAttribute(e, ATTR_HIDDEN)
        && !HasAttribute(e, ATTR_LFN));
}

static inline bool IsSystemFile(const DirEntry *e)
{
    return (HasAttribute(e, ATTR_SYSTEM)
        && !HasAttribute(e, ATTR_LFN));
}

static inline bool IsVolumeLabel(const DirEntry *e)
{
    return (HasAttribute(e, ATTR_LABEL)
        && !HasAttribute(e, ATTR_LFN));
}

static inline bool IsDirectory(const DirEntry *e)
{
    return HasAttribute(e, ATTR_DIRECTORY);
}

static inline bool IsDeviceFile(const DirEntry *e)
{
    // I'm not sure if this is even a thing
    return HasAttribute(e, ATTR_DEVICE);
}

static inline bool IsLongFileName(const DirEntry *e)
{
    return HasAttribute(e, ATTR_LFN)
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

#endif // FAT_HPP
