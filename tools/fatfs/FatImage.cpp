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

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include "FatImage.hpp"

FatImage::FatImage()
{
    m_bootSect = nullptr;
    m_rootDir = nullptr;
    m_allocTable = nullptr;
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
    if (m_rootDir) delete[] m_rootDir;
    if (m_allocTable) delete[] m_allocTable;
}

bool FatImage::Create(const std::string &filename)
{
    // TODO: disk geometry params

    if (m_file.is_open()) {
        m_file.close();
    }

    auto openMode = std::ios_base::in | std::ios_base::out | std::ios_base::binary;
    auto createMode = openMode | std::ios_base::trunc;

    m_file.open(filename, openMode);
    if (!m_file.is_open()) {
        m_file.open(filename, createMode);
    }
    if (!m_file.good()) {
        printf("fatfs: error: failed to create disk image file\n");
        return false;
    }

    CreateBootSector();
    CreateFileAllocTable();
    CreateRootDirectory();

    if (!WriteBootSector()) return false;
    if (!WriteFileAllocTable()) return false;
    if (!WriteRootDirectory()) return false;
    if (!ZeroData()) return false;

    // Override destructor write-out since it's already done
    m_bootSectNeedsUpdate = false;
    m_allocTableNeedsUpdate = false;
    m_rootDirNeedsUpdate = false;

    const char *name = filename.c_str();
    int sectors = GetParamBlock()->SectorCount;
    int size = sectors * GetParamBlock()->SectorSize;
    printf("Created '%s': sectors = %d, size = %d\n", name, sectors, size);

    return true;
}

bool FatImage::Load(const std::string &filename)
{
    if (m_file.is_open()) {
        m_file.close();
    }

    auto mode = std::ios_base::in | std::ios_base::out | std::ios_base::binary;
    m_file.open(filename, mode);
    if (!m_file.good()) {
        printf("fatfs: error: failed to open disk image file\n");
        return false;
    }

    if (!LoadBootSector()) return false;
    if (!LoadFileAllocTable()) return false;
    if (!LoadRootDirectory()) return false;

    return true;
}

bool FatImage::AddFile(const std::string &filename)
{
    std::fstream newFile;
    auto mode = std::ios_base::in | std::ios_base::out | std::ios_base::binary;
    
    newFile.open(filename, mode);
    if (!m_file.good()) {
        printf("fatfs: error: failed to open file '%s'\n", filename.c_str());
        return false;
    }
    
    // TODO: support nested directories

    DirectoryEntry *dirEntry = nullptr;
    for (int i = 0; i < GetParamBlock()->MaxRootDirEntries; i++) {
        char c = m_rootDir[i].FileName[0];
        if (c == 0x00 || c == 0xE5) {
            dirEntry = &m_rootDir[i];
            break;
        }
    }

    if (dirEntry == nullptr) {
        printf("fatfs: error: root directory is full!\n");
        return false;
    }

    std::string s(filename);
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    std::string name = s.substr(s.find_last_of("/\\") + 1);
    std::string extension = "";
    if (name.find('.') != std::string::npos) {
        int dotPos = name.find_last_of(".");
        extension = name.substr(dotPos + 1);
        name = name.substr(0, dotPos);
    }

    WriteString(dirEntry->FileName, name, FILENAME_LENGTH);
    WriteString(dirEntry->FileExtension, extension, EXTENSION_LENGTH);

    // TODO
    dirEntry->CreationDate.Year = 0;
    dirEntry->CreationDate.Month = 0;
    dirEntry->CreationDate.Day = 0;
    dirEntry->CreationTime.Hours = 0;
    dirEntry->CreationTime.Minutes = 0;
    dirEntry->CreationTime.Seconds = 0;
    dirEntry->ModifiedDate.Year = 0;
    dirEntry->ModifiedDate.Month = 0;
    dirEntry->ModifiedDate.Day = 0;
    dirEntry->ModifiedTime.Hours = 0;
    dirEntry->ModifiedTime.Minutes = 0;
    dirEntry->ModifiedTime.Seconds = 0;
    dirEntry->FileAttributes = 0;
    dirEntry->_Reserved0 = 0;
    dirEntry->_Reserved1 = 0;
    dirEntry->_Reserved2 = 0;
    dirEntry->_Reserved3 = 0;

    newFile.seekg(0, std::ios_base::end);
    dirEntry->FileSize = newFile.tellg();
    newFile.seekg(0, std::ios_base::beg);

    int sectorSize = GetParamBlock()->SectorSize;
    int bytesRemaining = dirEntry->FileSize;
    char buf[sectorSize];

    int firstCluster = -1;
    int lastCluster = -1;
    int currCluster = 0;
    int numClusters = 0;

    // TODO: alloc table/disk space bounds check

    while (bytesRemaining > 0) {
        while (GetClusterValue(currCluster) != 0) {
            currCluster++;
        }
        if (lastCluster > 0) {
            SetClusterValue(lastCluster, currCluster);
        }
        if (firstCluster < 0) {
            firstCluster = currCluster;
        }

        memset(buf, 0, sectorSize);
        newFile.read(buf, std::min(sectorSize, bytesRemaining));
        if (!newFile.good()) {
            printf("fatfs: error: failed to read file\n");
            return false;
        }

        bytesRemaining -= newFile.gcount();
        WriteDataCluster(currCluster, buf);
        numClusters++;

        lastCluster = currCluster;
        currCluster++;
    }

    SetClusterValue(lastCluster, -1);
    dirEntry->FileBegin = firstCluster;

    m_rootDirNeedsUpdate = true;
    return true;
}

BiosParameterBlock * FatImage::GetParamBlock() const
{
    return &m_bootSect->Params;
}

int FatImage::GetFirstFileAllocSectorNumber() const
{
    return GetParamBlock()->ReservedSectorCount;
}

int FatImage::GetFirstRootDirSectorNumber() const
{
    int sector = GetFirstFileAllocSectorNumber();
    sector += GetParamBlock()->SectorsPerTable * GetParamBlock()->TableCount;
    
    return sector;
}

int FatImage::GetFirstDataSectorNumber() const
{
    int sector = GetFirstRootDirSectorNumber();
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

    m_bootSect = new BootSector();
    SetOemName("fatfs");
    SetVolumeLabel("NO NAME");
    SetFileSystemType("FAT12");
    GetParamBlock()->VolumeId = (uint32_t) time(NULL);
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

    // printf("Writing boot sector...\n");
    m_file.seekp(0, std::ios_base::beg);
    m_file.write((char *) m_bootSect, sizeof(BootSector));
    if (!m_file.good()) {
        printf("fatfs: error: failed to write boot sector\n");
        return false;
    }

    return true;
}

void FatImage::CreateFileAllocTable()
{
    size_t fatSize = GetParamBlock()->SectorsPerTable * GetParamBlock()->SectorSize;
    m_allocTable = new char[fatSize]();

    SetClusterValue(0, 0xFF0);  // Reserved: ??? (endianness marker?)
    SetClusterValue(1, 0xFFF);  // Reserved: End-of-chain value
}

bool FatImage::LoadFileAllocTable()
{
    if (m_allocTable) {
        delete[] m_allocTable;
    }

    size_t fatSize = GetParamBlock()->SectorsPerTable * GetParamBlock()->SectorSize;
    m_allocTable = new char[fatSize];

    for (int i = 0; i < GetParamBlock()->TableCount; i++) {
        // Read-in all FATs, keep the last one
        m_file.read((char *) m_allocTable, fatSize);
        if (!m_file.good()) {
            printf("fatfs: error: failed to read file allocation table\n");
            return false;
        }
    }

    return true;
}

bool FatImage::WriteFileAllocTable()
{
    if (!m_allocTable) {
        return false;
    }

    long seekOffset = GetFirstFileAllocSectorNumber() * GetParamBlock()->SectorSize;
    size_t allocTableSize = GetParamBlock()->SectorsPerTable * GetParamBlock()->SectorSize;

    // printf("Writing file allocation table...\n");
    m_file.seekp(seekOffset, std::ios_base::beg);
    for (int i = 0; i < GetParamBlock()->TableCount; i++) {
        m_file.write((char *) m_allocTable, allocTableSize);
        if (!m_file.good()) {
            printf("fatfs: error: failed to write file allocation table\n");
            return false;
        }
    }

    return true;
}

void FatImage::CreateRootDirectory()
{
    m_rootDir = new DirectoryEntry[GetParamBlock()->MaxRootDirEntries]();
}

bool FatImage::LoadRootDirectory()
{
    if (m_rootDir) {
        delete[] m_rootDir;
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
    if (!m_rootDir) {
        return false;
    }

    int numEntries = GetParamBlock()->MaxRootDirEntries;
    size_t rootDirSize = numEntries * sizeof(DirectoryEntry);
    long seekOffset = GetFirstRootDirSectorNumber() * GetParamBlock()->SectorSize; 

    // printf("Writing root directory...\n");
    m_file.seekp(seekOffset, std::ios_base::beg);
    m_file.write((char *) m_rootDir, rootDirSize);
    if (!m_file.good()) {
        printf("fatfs: error: failed to write root directory\n");
        return false;
    }

    return true;
}

bool FatImage::WriteDataCluster(int num, char *data)
{
    int sectorSize = GetParamBlock()->SectorSize;
    int sectorsPerCluster = GetParamBlock()->SectorsPerCluster;
    int clusterSize = sectorSize * sectorsPerCluster;
    int clusterCount = GetParamBlock()->SectorCount /sectorsPerCluster;
    int originalNum = num;

    // Clusters 0 and 1 are reserved, data clusters actually begin at 2
    num -= 2;
    if (num < 0 || num >= clusterCount) {
        printf("fatfs: error: invalid cluster number - %d\n", num);
        return false;
    }

    long seekOffset = (GetFirstDataSectorNumber() * sectorSize) + (num * clusterSize);
    m_file.seekp(seekOffset, std::ios_base::beg);

    // printf("Writing cluster %d\n", originalNum);
    m_file.write(data, sectorSize);
    if (!m_file.good()) {
        printf("fatfs: error: failed to write cluster %d\n", originalNum);
        return false;
    }

    return true;
}

uint16_t FatImage::GetClusterValue(int num)
{
    // FAT12
    // TODO: FAT16

    int index = num + (num / 2);
    uint16_t value = *(uint16_t *) &m_allocTable[index];

    if (num & 1) {
        value >>= 4;
    }
    return value & 0x0FFF;
    
}

void FatImage::SetClusterValue(int num, uint16_t value)
{
    // FAT12
    // TODO: FAT16

    int index = num + (num / 2);
    uint16_t tableValue = *(uint16_t *) &m_allocTable[index];

    if (num & 1) {
        tableValue = ((value << 4) & 0xFFF0) | (tableValue & 0x000F);
    }
    else {
        tableValue = (tableValue & 0xF000) | (value & 0x0FFF);
    }

    *(uint16_t *) &m_allocTable[index] = tableValue;
    m_allocTableNeedsUpdate = true;
}

bool FatImage::ZeroData()
{
    int sectorSize = GetParamBlock()->SectorSize;
    char zeroBuf[sectorSize] = { 0 };

    long seekOffset = GetFirstDataSectorNumber() * sectorSize;
    m_file.seekp(seekOffset, std::ios_base::beg);

    int count = GetParamBlock()->SectorCount - GetFirstDataSectorNumber();
    for (int i = 0; i < count; i++) {
        m_file.write(zeroBuf, sectorSize);
        if (!m_file.good()) {
            printf("fatfs: error: failed to write cluster %d\n", i);
            return false;
        }
    }

    return true;
}

void FatImage::WriteString(char *dest, const std::string &source, int length)
{
    std::string s = source;
    s.resize(length, ' ');
    strncpy(dest, s.c_str(), length);
}
