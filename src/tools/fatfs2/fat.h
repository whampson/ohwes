/*******************************************************************************
 *    File: fat.hpp
 *  Author: Wes Hampson
 * Created: 18 Nov 2022
 *
 * FAT12 and FAT16 file system support.
 *******************************************************************************/

#ifndef FAT_H
#define FAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <inttypes.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NAME_LENGTH                 8
#define EXTENSION_LENGTH            3
#define LABEL_LENGTH                11
#define SHORTNAME_LENGTH            (LABEL_LENGTH+1)
#define LONGNAME_LENGTH             260

#define MAX_NAME                    (NAME_LENGTH+1)
#define MAX_EXTENSION               (EXTENSION_LENGTH+1)
#define MAX_LABEL                   (LABEL_LENGTH+1)
#define MAX_SHORTNAME               (SHORTNAME_LENGTH+1)
#define MAX_LONGNAME                (LONGNAME_LENGTH+1)

#define MEDIATYPE_1440K             0xF0
#define MEDIATYPE_FIXED             0xF8

#define BOOTSIG                     0xAA55
#define BPBSIG_DOS40                0x28
#define BPBSIG_DOS41                0x29

#define FIRST_CLUSTER               2
#define LAST_CLUSTER_12             0xFF6
#define LAST_CLUSTER_16             0xFFF6
#define MIN_CLUSTER_12              1
#define MAX_CLUSTER_12              (LAST_CLUSTER_12-FIRST_CLUSTER)
#define MIN_CLUSTER_16              (MAX_CLUSTER_12+1)
#define MAX_CLUSTER_16              (LAST_CLUSTER_16-FIRST_CLUSTER)

#define CLUSTER_FREE                0
#define CLUSTER_RESERVED            1
#define CLUSTER_BAD                 0xFFF7
#define CLUSTER_EOC                 0xFFFF

#define MIN_SECTOR_SIZE             512
#define MAX_SECTOR_SIZE             32768

#define MIN_SEC_PER_CLUST           1
#define MAX_SEC_PER_CLUST           128

static_assert(MAX_CLUSTER_12 == 4084, "Bad max FAT12 cluster size!");
static_assert(MAX_CLUSTER_16 == 65524, "Bad max FAT16 cluster size!");

// -----------------------------------------------------------------------------
// String Functions
// -----------------------------------------------------------------------------

/**
 * Reads at most n characters into dst from src. Leading and trailing spaces
 * will be trimmed. A NUL terminator will be added to dst. The number of
 * characters written into dst will be returned.
 *
 * The FAT file system stores ASCII strings padded with trailing spaces and no
 * NUL terminator. Use this function to create a standard C string from a FAT
 * string.
 */
int ReadFatString(char *dst, const char *src, int n);

/**
 * Writes n characters into dst from src. If src is less than n characters, dst
 * will be padded with spaces. No NUL terminator will be added to dst. The
 * number of characters written will be returned.
 *
 * The FAT file system stores ASCII strings padded with trailing spaces and no
 * NUL terminator. Use this function to create a FAT string from a standard C
 * string.
 */
int WriteFatString(char *dst, const char *src, int n);


// -----------------------------------------------------------------------------
// BIOS Parameter Block
// -----------------------------------------------------------------------------

/**
 * BIOS Parameter Block
 * Contains disk and volume information.
 *
 * This is the MS-DOS 4.0/4.1 version of the BPB, which is the most common
 * format these days.
 *
 * https://jdebp.uk/FGA/bios-parameter-block.html
 * https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#BPB
 */
typedef struct BiosParamBlock
{
    // DOS 2.0 BPB
    uint16_t SectorSize;            // Size of a sector in bytes
    uint8_t SectorsPerCluster;      // Number of sectors per data cluster
    uint16_t ReservedSectorCount;   // Number of sectors in the reserved area
    uint8_t TableCount;             // Number of File Allocation Tables on disk
    uint16_t RootDirCapacity;       // Number of entries allowed in the root directory
    uint16_t SectorCount;           // Total number of sectors on disk
    uint8_t MediaType;              // Physical media type identifier
    uint16_t SectorsPerTable;       // Number of sectors per File Allocation Table

    // DOS 3.31 BPB
    uint16_t SectorsPerTrack;       // Number of sectors per physical track on disk
    uint16_t HeadCount;             // Number of physical heads on disk
    uint32_t HiddenSectorCount;     // Number of hidden sectors, not supported unless disk is partitioned
    uint32_t SectorCountLarge;      // Total number of sectors on disk if 'SectorCount' exceeds 16 bits

    // DOS 4.1 BPB
    uint8_t DriveNumber;            // Disk drive number for BIOS I/O purposes
    uint8_t _Reserved;              // Reserved; MSDOS uses this for chkdsk
    uint8_t Signature;              // BPB format version signature
    uint32_t VolumeId;              // Volume serial number
    char Label[LABEL_LENGTH];       // Volume label
    char FsType[NAME_LENGTH];       // Do not use for file system type identification
} __attribute__((packed)) BiosParamBlock;

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

void InitBiosParamBlock(BiosParamBlock *bpb);

// -----------------------------------------------------------------------------
// Boot Sector
// -----------------------------------------------------------------------------

/**
 * Boot Sector
 * Contains the initial boot code and volume information.
 */
typedef struct BootSector
{
    char JumpCode[3];               // Small bit of code that jumps to boot loader code
    char OemName[NAME_LENGTH];      // Format utility identifier
    BiosParamBlock BiosParams;      // BIOS Parameter Block
    char BootCode[448];             // Boot loader code
    uint16_t BootSignature;         // Boot sector signature, indicating whether the disk is bootable
} BootSector;

static_assert(offsetof(BootSector, JumpCode) == 0x000, "Bad JumpCode offset!");
static_assert(offsetof(BootSector, OemName) == 0x003, "Bad OemName offset!");
static_assert(offsetof(BootSector, BiosParams) == 0x00B, "Bad BiosParams offset!");
static_assert(offsetof(BootSector, BootCode) == 0x03E, "Bad BootCode offset!");
static_assert(offsetof(BootSector, BootSignature) == 0x1FE, "Bad BootSignature offset!");
static_assert(sizeof(BootSector) == 512, "Bad BootSector size!");

void InitBootSector(
    BootSector *bootSect,
    const BiosParamBlock *bpb,
    const char *oemName);

// -----------------------------------------------------------------------------
// File Allocation Table
// -----------------------------------------------------------------------------

void InitFat12(char *fat, size_t sizeBytes, uint8_t mediaType);
void InitFat16(char *fat, size_t sizeBytes, uint8_t mediaType);

uint32_t GetCluster12(const char *fat, uint32_t index);
uint32_t GetCluster16(const char *fat, uint32_t index);

uint32_t SetCluster12(char *fat, uint32_t index, uint32_t value);
uint32_t SetCluster16(char *fat, uint32_t index, uint32_t value);

// -----------------------------------------------------------------------------
// Date/Time
// -----------------------------------------------------------------------------

/**
 * FAT Date Structure
*/
typedef struct FatDate
{
    uint16_t Day       : 5;         // Day of month: 1-31
    uint16_t Month     : 4;         // Month of year: 1-12
    uint16_t Year      : 7;         // Calendar year: 0-127 (0 = 1980)
} FatDate;
static_assert(sizeof(FatDate) == 2, "Bad FatDate size!");

/**
 * FAT Time Structure
*/
typedef struct FatTime
{
    uint16_t Seconds   : 5;         // Seconds: 0-29 (secs/2)
    uint16_t Minutes   : 6;         // Minutes: 0-59
    uint16_t Hours     : 5;         // Hours: 0-23
} FatTime;
static_assert(sizeof(FatTime) == 2, "Bad FatTime size!");

void GetDate(struct tm *dst, const FatDate *src);       // TODO: return dst?
void SetDate(FatDate *dst, const struct tm *src);

void GetTime(struct tm *dst, const FatTime *src);
void SetTime(FatTime *dst, const struct tm *src);

// -----------------------------------------------------------------------------
// File Attributes
// -----------------------------------------------------------------------------

/**
 * File Attributes
*/
enum FileAttrs
{
    ATTR_READONLY   = 1 << 0,           // Read-Only
    ATTR_HIDDEN     = 1 << 1,           // Hidden
    ATTR_SYSTEM     = 1 << 2,           // System File
    ATTR_LABEL      = 1 << 3,           // Volume Label
    ATTR_DIRECTORY  = 1 << 4,           // Directory
    ATTR_ARCHIVE    = 1 << 5,           // Archived (used as a dirty bit for backup utilities)
    ATTR_DEVICE     = 1 << 6,           // Device file (not usually found on disk)
    ATTR_LFN        = ATTR_LABEL  |     // Long File Name
                      ATTR_SYSTEM |
                      ATTR_HIDDEN |
                      ATTR_READONLY
};

// -----------------------------------------------------------------------------
// Directory Entry
// -----------------------------------------------------------------------------

/**
 * Directory Entry
 * Contains file size, location, attribute, and timestamp information.
*/
typedef struct DirEntry
{
    char Label[LABEL_LENGTH];       // File name and extension or volume label
    uint8_t Attributes;             // File attributes
    uint8_t _Reserved1;             // Reserved; varies by system
    uint8_t _Reserved2;             // Reserved; used for fine creation time, 10ms increments: 0-199
    FatTime CreationTime;           // File creation time
    FatDate CreationDate;           // File creation fate
    FatDate AccessedDate;           // File accessed date
    uint16_t _Reserved3;            // Reserved; used by some systems for access control
    FatTime ModifiedTime;           // File modified time
    FatDate ModifiedDate;           // File modifed date
    uint16_t FirstCluster;          // First cluster index
    uint32_t FileSize;              // File size in bytes, zero for directories and volume labels
} DirEntry;
static_assert(sizeof(DirEntry) == 32, "Bad DirEntry size!");

void InitDirEntry(DirEntry *e);

time_t GetCreationTime(struct tm *dst, const DirEntry *src);
void SetCreationTime(DirEntry *dst, const struct tm *src);

time_t GetModifiedTime(struct tm *dst, const DirEntry *src);
void SetModifiedTime(DirEntry *dst, const struct tm *src);

time_t GetAccessedTime(struct tm *dst, const DirEntry *src);
void SetAccessedTime(DirEntry *dst, const struct tm *src);

/**
 * Gets the short file name from a directory entry. Returns a pointer to dst
 * for convenience.
*/
char * GetShortName(char dst[MAX_SHORTNAME], const DirEntry *src);

/**
 * Sets the short file name in a directory entry. Returns a boolean value
 * indicating whether the input string was a valid short name.
 *
 * A short file name is limited to 8 characters, followed by an optional dot (.)
 * and an extension of up to 3 characters. This function will return false if
 * any of these limits are exceeded, or if an invalid character is found in
 * either the name or extension. A short name may consist of any combination of
 * letters, digits, characters with a code point value greater than 127, or the
 * following symbols: $ % ' - _ @ ~ ` ! ( ) { } ^ # &
 *
 * All letters will be converted to uppercase and their original case will be
 * lost.
*/
bool SetShortName(DirEntry *dst, const char *src);

static inline bool HasAttribute(const DirEntry *src, uint8_t attr)
{
    return (src->Attributes & attr) == attr;
}

static inline void ClearAttribute(DirEntry *dst, uint8_t attr)
{
    dst->Attributes &= ~attr;
}

static inline void SetAttribute(DirEntry *dst, uint8_t attr)
{
    dst->Attributes |= attr;
}

static inline bool IsReadOnly(const DirEntry *src)
{
    return (HasAttribute(src, ATTR_READONLY)
        && !HasAttribute(src, ATTR_LFN));
}

static inline bool IsHidden(const DirEntry *src)
{
    return (HasAttribute(src, ATTR_HIDDEN)
        && !HasAttribute(src, ATTR_LFN));
}

static inline bool IsSystemFile(const DirEntry *src)
{
    return (HasAttribute(src, ATTR_SYSTEM)
        && !HasAttribute(src, ATTR_LFN));
}

static inline bool IsLabel(const DirEntry *src)
{
    return (HasAttribute(src, ATTR_LABEL)
        && !HasAttribute(src, ATTR_LFN));
}

static inline bool IsDirectory(const DirEntry *src)
{
    return HasAttribute(src, ATTR_DIRECTORY);
}

static inline bool IsDeviceFile(const DirEntry *src)
{
    // I'm not sure if this is even a thing
    return HasAttribute(src, ATTR_DEVICE);
}

static inline bool IsArchive(const DirEntry *src)
{
    return HasAttribute(src, ATTR_ARCHIVE);
}

static inline bool IsLongFileName(const DirEntry *src)
{
    return HasAttribute(src, ATTR_LFN)
        && src->FirstCluster == 0;
}

static inline bool IsDeleted(const DirEntry *src)
{
    return ((uint8_t) src->Label[0]) == 0xE5;
}

static inline bool IsFree(const DirEntry *src)
{
    return IsDeleted(src) || src->Label[0] == 0x00;
}

static inline bool IsRoot(const DirEntry *src)
{
    return IsDirectory(src) && src->FirstCluster == 0;
}

static inline bool IsCurrentDirectory(const DirEntry *src)
{
    char buf[MAX_SHORTNAME];
    GetShortName(buf, src);

    return strcmp(buf, ".") == 0;
}

static inline bool IsParentDirectory(const DirEntry *src)
{
    char buf[MAX_SHORTNAME];
    GetShortName(buf, src);

    return strcmp(buf, "..") == 0;
}

// -----------------------------------------------------------------------------
// Long File Name
// A "hack" on the DirEntry structure above to allow for long file names. To
// facilitate this, the Attributes field is set to LABEL|SYSTEM|HIDDEN|READONLY,
// a combination not expected by old file system tools and thus ignored if LFNs
// are not supported. This allows for the remaining 31 bytes to be repurposed,
// mostly. Each LFN chunk contains a checksum for verifying its validity with
// the corresponding 8.3 filename (which immediately follows the LFN chain), as
// well as a sequence number which counts down as you read the directory. Thus,
// the last characters in the LFN are stored first on disk. The final entry in
// the LFN chain has bit 6 set in the sequence number; bit 5 is always zero, and
// bits 4-0 are used for the actual sequence number. However, a maximum of only
// 20 entries is allowed in the chain. Why? Ask Microsoft. At 13 UCS-2
// characters per chunk, the maximum long file name length is 260 characters.
// Conveniently, this is equivalent to the MAX_PATH constant on Microsoft
// systems. A deleted entry uses 0xE5 as the sequence number which coincides
// with the first character of the file name on regular directory entires.
// Clever!
// -----------------------------------------------------------------------------

/**
 * Long File Name
*/
typedef struct LongFileName
{
    struct {
        uint8_t SequenceNumber : 6; // LFN chunk index
        uint8_t FirstInChain : 1;   // Set if this chunk is the first in the chain
        uint8_t _Reserved1 : 1;     // Reserved; do not use
    };
    uint16_t Name1[5];              // Characters 1-5 of this LFN chunk
    uint8_t Attributes;             // Always ATTR_LFN for LFNs
    uint8_t _Reserved2;             // Reserved; varies by system
    uint8_t Checksum;               // 8.3 name checksum
    uint16_t Name2[6];              // Characters 6-11 of this LFN chunk
    uint16_t FirstCluster;          // Always 0 for LFNs
    uint16_t Name3[2];              // Characters 12-13 of this LFN chunk
} __attribute__((packed)) LongFileName;

static_assert(sizeof(LongFileName) == sizeof(DirEntry), "Bad LongFileName size!");
static_assert(offsetof(LongFileName, Name1) == 0x01, "Bad Name1 offset!");
static_assert(offsetof(LongFileName, Attributes) == 0x0B, "Bad Attributes offset!");
static_assert(offsetof(LongFileName, _Reserved2) == 0x0C, "Bad _Reserved2 offset!");
static_assert(offsetof(LongFileName, Checksum) == 0x0D, "Bad Checksum offset!");
static_assert(offsetof(LongFileName, Name2) == 0x0E, "Bad Name2 offset!");
static_assert(offsetof(LongFileName, FirstCluster) == 0x1A, "Bad FirstCluster offset!");
static_assert(offsetof(LongFileName, Name3) == 0x1C, "Bad Name3 offset!");

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// WARNING: these functions will read AT MOST 19 * sizeof(DirEntry) bytes ahead
// of the src pointer. Please ensure that src points to a buffer containing an
// entire directory table (not a single DirEntry!) and that src points to the
// first entry in an LFN chain. If successful, the advanced pointer, which
// should point to the shortname DirEntry, will be returned. However, if the src
// pointer is not a valid LFN chain first entry, src will be returned and dst
// will be unmodified. If a checksum mismatch is found while reading the LFN,
// dst will contain an empty string and a pointer to the shortname entry will
// be returned.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

const DirEntry * GetLongName(wchar_t dst[MAX_LONGNAME], const DirEntry *srcTable);
// const DirEntry * SetLongName(DirEntry *dstTable, wchar_t *src);

#ifdef __cplusplus
}       // extern "C"
#endif

#endif // FAT_H
