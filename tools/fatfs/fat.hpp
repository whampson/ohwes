/*============================================================================*
 * Copyright (C) 2020 Wes Hampson. All Rights Reserved.                       *
 *                                                                            *
 * This file is part of the Niobium Operating System.                         *
 * Niobium is free software; you may redistribute it and/or modify it under   *
 * the terms of the license agreement provided with this software.            *
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
 *----------------------------------------------------------------------------*
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
    struct {    // Extended BIOS Parameter Block
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
    char FileName[FILENAME_LENGTH];
    char FileExtension[EXTENSION_LENGTH];
    uint8_t FileAttributes;
    uint8_t _Reserved0;
    uint8_t _Reserved1;
    struct {
        uint16_t Seconds   : 5;
        uint16_t Minutes   : 6;
        uint16_t Hours     : 5;
    } CreationTime;
    struct {
        uint16_t Day       : 5;
        uint16_t Month     : 4;
        uint16_t Year      : 7;
    } CreationDate;
    uint16_t _Reserved2;
    uint16_t _Reserved3;
    struct {
        uint16_t Seconds   : 5;
        uint16_t Minutes   : 6;
        uint16_t Hours     : 5;
    } ModifiedTime;
    struct {
        uint16_t Day       : 5;
        uint16_t Month     : 4;
        uint16_t Year      : 7;
    } ModifiedDate;
    uint16_t FileBegin;
    uint32_t FileSize;
} DirectoryEntry;

static_assert(sizeof(BiosParameterBlock) == 51, "Invalid BIOS Parameter Block size!");
static_assert(sizeof(BootSector) == 512, "Invalid boot sector size!");
static_assert(sizeof(DirectoryEntry) == 32, "Invalid boot sector size!");

#endif  // __FAT_HPP
