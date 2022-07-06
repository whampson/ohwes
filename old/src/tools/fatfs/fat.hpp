/*============================================================================*
 * Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                  *
 *                                                                            *
 * This file is part of the OHWES Operating System.                           *
 * OHWES is free software; you may redistribute it and/or modify it under the *
 * terms of the license agreement provided with this software.                *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: tools/fatfs/fat.hpp                                               *
 * Created: November 28, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *                                                                            *
 * Structure definitions for the FAT filesystem.                              *
 * FAT12 and FAT16 only; FAT32 is not supported.                              *
 *============================================================================*/

#ifndef __FAT_HPP
#define __FAT_HPP

#include <cinttypes>

#define JUMPCODE_SIZE       3
#define BOOTCODE_SIZE       448
#define OEMNAME_LENGTH      8
#define FSTYPE_LENGTH       8
#define LABEL_LENGTH        11
#define FILENAME_LENGTH     8
#define EXTENSION_LENGTH    3

#pragma pack(1)
typedef struct
{
    uint16_t SectorSize;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectorCount;
    uint8_t TableCount;
    uint16_t MaxRootDirEntries;
    uint16_t SectorCount;
    uint8_t MediaType;
    uint16_t SectorsPerTable;
    uint16_t SectorsPerTrack;
    uint16_t HeadCount;
    uint32_t HiddenSectorCount;
    uint32_t LargeSectorCount;
    struct { // Extended BIOS Parameter Block
        uint8_t DriveNumber;
        uint8_t _Reserved;
        uint8_t ExtendedBootSignature;
        uint32_t VolumeId;
        char Label[LABEL_LENGTH];
        char FileSystemType[FSTYPE_LENGTH];
    };
} BiosParameterBlock;

typedef struct
{
    char JumpCode[JUMPCODE_SIZE];
    char OemName[OEMNAME_LENGTH];
    BiosParameterBlock Params;
    char BootCode[BOOTCODE_SIZE];
    uint16_t BootSignature;
} BootSector;

typedef struct
{
    uint16_t Day       : 5;     // 1-31
    uint16_t Month     : 4;     // 1-12
    uint16_t Year      : 7;     // 0-127 (0 = 1980)
} FatDate;

typedef struct
{
    uint16_t Seconds   : 5;     // 0-29 (secs/2)
    uint16_t Minutes   : 6;     // 0-59
    uint16_t Hours     : 5;     // 0-23
} FatTime;

enum FileAttrs : uint8_t
{
    ATTR_RO     = 0x01,     // Read Only
    ATTR_HID    = 0x02,     // Hidden
    ATTR_SYS    = 0x04,     // System File
    ATTR_VL     = 0x08,     // Volume Label
    ATTR_DIR    = 0x10,     // Directory
    ATTR_AR     = 0x20,     // Archive
    ATTR_DEV    = 0x40,     // Device File
};

typedef struct
{
    char Name[FILENAME_LENGTH];
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

static_assert(sizeof(BiosParameterBlock) == 51, "Invalid BIOS Parameter Block size!");
static_assert(sizeof(BootSector) == 512, "Invalid Boot Sector size!");
static_assert(sizeof(DirectoryEntry) == 32, "Invalid Directory Entry size!");

#endif  // __FAT_HPP
