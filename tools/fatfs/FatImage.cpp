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
 *    File: tools/fatfs/FatImage.cpp                                          *
 * Created: November 29, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#include <string.h>
#include "FatImage.hpp"

FatImage::FatImage()
{
    m_bootSect = nullptr;
    m_rootDir = nullptr;
    m_allocTable = nullptr;
    m_isFileOpen = false;
    m_bootSectNeedsUpdate = false;
    m_allocTableNeedsUpdate = false;
    m_rootDirNeedsUpdate = false;
}

FatImage::~FatImage()
{
    if (m_bootSectNeedsUpdate) {
        WriteBootSector();
    }
    if (m_allocTableNeedsUpdate) {
        WriteFileAllocTable();
    }
    if (m_rootDirNeedsUpdate) {
        WriteRootDirectory();
    }

    if (m_bootSect) delete m_bootSect;
    if (m_rootDir) delete m_rootDir;
    if (m_allocTable) delete m_allocTable;
}

bool FatImage::Create(const std::string &filename)
{
    // TODO: disk geometry params

    auto mode = std::ios_base::in | std::ios_base::out | std::ios_base::binary | std::ios_base::trunc;
    m_file.open(filename, mode);
    if (!m_file.good()) {
        printf("fatfs: error: failed to create image file\n");
        return false;
    }

    CreateBootSector();
    CreateFileAllocTable();
    CreateRootDirectory();

    if (!WriteBootSector()) return false;
    if (!WriteFileAllocTable()) return false;
    if (!WriteRootDirectory()) return false;

    return true;
}

bool FatImage::Load(const std::string &filename)
{
    if (m_isFileOpen) {
        return false;
    }

    auto mode = std::ios_base::in | std::ios_base::out | std::ios_base::binary;
    m_file.open(filename, mode);
    if (!m_file.good()) {
        printf("fatfs: error: failed to open image file\n");
        return false;
    }

    if (!LoadBootSector()) return false;
    if (!LoadFileAllocTable()) return false;
    if (!LoadRootDirectory()) return false;

    m_isFileOpen = true;
    return true;
}

BiosParameterBlock * FatImage::GetParamBlock() const
{
    return &m_bootSect->Params;
}

int FatImage::GetDataStartSector() const
{
    int sector = 0;
    sector += GetParamBlock()->ReservedSectorCount;
    sector += GetParamBlock()->SectorsPerTable * GetParamBlock()->TableCount;
    sector += GetParamBlock()->MaxRootDirEntries * sizeof(DirectoryEntry) / GetParamBlock()->SectorSize;
    
    return sector;
}

std::string FatImage::GetVolumeLabel() const
{
    return std::string(GetParamBlock()->Label, LABEL_LENGTH);
}

void FatImage::SetVolumeLabel(const std::string &label)
{
    WriteString(GetParamBlock()->Label, label, LABEL_LENGTH);
    m_bootSectNeedsUpdate = true;
}

std::string FatImage::GetOemName() const
{
    return std::string(m_bootSect->OemName, OEMNAME_LENGTH);
}

void FatImage::SetOemName(const std::string &name)
{
    WriteString(m_bootSect->OemName, name, OEMNAME_LENGTH);
    m_bootSectNeedsUpdate = true;
}

std::string FatImage::GetFileSystemType() const
{
    return std::string(GetParamBlock()->FileSystemType, FSTYPE_LENGTH);
}

void FatImage::SetFileSystemType(const std::string &name)
{
    WriteString(GetParamBlock()->FileSystemType, name, FSTYPE_LENGTH);
    m_bootSectNeedsUpdate = true;
}

void FatImage::CreateBootSector()
{
    // TODO: disk geometry params
    int cyl = 80;
    int hed = 2;
    int sec = 18;

    if (m_bootSect) {
        delete m_bootSect;
    }
    
    m_bootSect = new BootSector;
    SetOemName("fatfs");
    SetVolumeLabel("NO NAME");
    SetFileSystemType("FAT12");
    GetParamBlock()->VolumeId = 0xFEEDFACE;
    GetParamBlock()->DriveNumber = 0;
    GetParamBlock()->MediaType = 0xF0;             // 3.5in floppy
    GetParamBlock()->HeadCount = hed;
    GetParamBlock()->SectorsPerTrack = sec;
    GetParamBlock()->SectorCount = cyl * hed * sec;
    GetParamBlock()->SectorSize = 512;
    GetParamBlock()->SectorsPerCluster = 1;
    GetParamBlock()->SectorsPerTable = 9;
    GetParamBlock()->TableCount = 2;
    GetParamBlock()->MaxRootDirEntries = 224;
    GetParamBlock()->ReservedSectorCount = 1;
    GetParamBlock()->HiddenSectorCount = 0;
    GetParamBlock()->LargeSectorCount = 0;
    GetParamBlock()->ExtendedBootSignature = 0x29;
    GetParamBlock()->_Reserved = 0;
    m_bootSect->BootSignature = 0xAA55;

    char jumpcode[JUMPCODE_SIZE] = {
        '\xEB', '\x3C',     // entry:       jmp     boot_code
        '\x90'              //              nop
    };
    char bootcode[BOOTCODE_SIZE] = {
        "\x0E"              // boot_code:   pushw   %cs
        "\x1F"              //              popw    %ds
        "\x8D\x36\x58\x7C"  //              leaw    message
        "\xB4\x0E"          //              movb    $0x0E, %ah
        "\xBB\x07"          //              movw    $0x07, %bx
        "\x00\xAC"          // print_loop:  lodsb
        "\x20\xC0"          //              andb    %al, %al
        "\x74\x04"          //              jz      key_press
        "\xCD\x10"          //              int     $0x10
        "\xEB\xF7"          //              jmp     print_loop
        "\x30\xE4"          // key_press:   xorb    %ah, %ah
        "\xCD\x16"          //              int     $0x16
        "\xCD\x19"          //              int     $0x19
                            // message:
        "This is not a bootable disk.\r\n"
        "Please insert a bootable disk and press any key to try again.\r\n"
    };

    memcpy(m_bootSect->JumpCode, jumpcode, JUMPCODE_SIZE);
    memcpy(m_bootSect->BootCode, bootcode, BOOTCODE_SIZE);
}

bool FatImage::LoadBootSector()
{
    if (m_bootSect) {
        delete m_bootSect;
    }
    
    m_bootSect = new BootSector;
    m_file.read((char *) m_bootSect, sizeof(BootSector));
    if (!m_file.good()) {
        printf("fatfs: error: failed to read boot sector\n");
        return false;
    }

    return true;
}

bool FatImage::WriteBootSector()
{
    if (!m_bootSect) {
        return false;
    }

    printf("Writing boot sector...\n");
    m_file.seekp(0, std::ios_base::beg);
    m_file.write((char *) m_bootSect, sizeof(BootSector));
    if (!m_file.good()) {
        printf("fatfs: error: failed to write bootsector\n");
        return false;
    }

    return true;
}

void FatImage::CreateFileAllocTable()
{
    
}

bool FatImage::LoadFileAllocTable()
{
    if (m_allocTable) {
        delete m_allocTable;
    }

    size_t fatSize = GetParamBlock()->SectorsPerTable * GetParamBlock()->SectorSize;
    m_allocTable = new char[fatSize];

    m_file.read((char *) m_allocTable, fatSize);
    if (!m_file.good()) {
        printf("fatfs: error: failed to read file allocation table\n");
        return false;
    }

    return true;
}

bool FatImage::WriteFileAllocTable()
{
    return true;
}

void FatImage::CreateRootDirectory()
{
    
}

bool FatImage::LoadRootDirectory()
{
    if (m_rootDir) {
        delete m_rootDir;
    }

    int numEntries = GetParamBlock()->MaxRootDirEntries;
    size_t rootDirSize = numEntries * sizeof(DirectoryEntry);
    m_rootDir = new DirectoryEntry[numEntries];

    m_file.read((char *) m_rootDir, rootDirSize);
    if (!m_file.good()) {
        printf("fatfs: error: failed to read root directory\n");
        return false;
    }

    return true;
}

bool FatImage::WriteRootDirectory()
{
    return true;
}

void FatImage::WriteString(char *dest, const std::string &source, int length)
{
    std::string s = source;
    s.resize(length, ' ');
    strncpy(dest, s.c_str(), length);
}
