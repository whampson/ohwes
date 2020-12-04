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
#include <cstdarg>
#include <ctime>
#include "helpers.hpp"
#include "FatImage.hpp"

FatImage::FatImage()
{
    Quiet = false;
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

int FatImage::GetSectorSize() const
{
    return GetParamBlock()->SectorSize;
}

int FatImage::GetClusterSize() const
{
    return GetSectorSize() * GetParamBlock()->SectorsPerCluster;
}

int FatImage::GetSectorCount() const
{
    return GetParamBlock()->SectorCount;
}

int FatImage::GetClusterCount() const
{
    return (GetSectorCount() - GetFirstDataSectorNumber()) / GetParamBlock()->SectorsPerCluster;
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
    SetString(GetParamBlock()->Label, label, LABEL_LENGTH);
    m_bootSectNeedsUpdate = true;
}

std::string FatImage::GetOemName() const
{
    return std::string(m_bootSect->OemName, OEMNAME_LENGTH);
}

void FatImage::SetOemName(const std::string &name)
{
    SetString(m_bootSect->OemName, name, OEMNAME_LENGTH);
    m_bootSectNeedsUpdate = true;
}

std::string FatImage::GetFileSystemType() const
{
    return std::string(GetParamBlock()->FileSystemType, FSTYPE_LENGTH);
}

void FatImage::SetFileSystemType(const std::string &name)
{
    SetString(GetParamBlock()->FileSystemType, name, FSTYPE_LENGTH);
    m_bootSectNeedsUpdate = true;
}

bool FatImage::Create(const std::string &path)
{
    // TODO: disk geometry params

    if (m_file.is_open()) {
        m_file.close();
    }

    auto openMode = std::ios_base::in | std::ios_base::out | std::ios_base::binary;
    auto createMode = openMode | std::ios_base::trunc;

    m_file.open(path, openMode);
    if (!m_file.is_open()) {
        m_file.open(path, createMode);
    }
    RIF_M(m_file.good(), "failed to create disk image\n");

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

    int sectSize = GetParamBlock()->SectorSize;
    int sectCount = GetParamBlock()->SectorCount;
    int size = sectSize * sectCount;
    int free = (sectCount - GetFirstDataSectorNumber()) * sectSize;

    PrintInfo("%s: sectors = %d, size = %d, free = %d\n",
        get_filename(path).c_str(),
        sectCount,
        size,
        free);

    return true;
}

bool FatImage::Load(const std::string &path)
{
    if (m_file.is_open()) {
        m_file.close();
    }

    auto mode = std::ios_base::in | std::ios_base::out | std::ios_base::binary;
    m_file.open(path, mode);
    RIF_MF(m_file.good(), "failed to open disk image '%s'\n", path.c_str());

    if (!LoadBootSector()) return false;
    if (!LoadFileAllocTable()) return false;
    if (!LoadRootDirectory()) return false;

    return true;
}

bool FatImage::AddFile(const std::string &srcPath)
{
    int sectorSize = GetParamBlock()->SectorSize;
    int clusterSize = sectorSize * GetParamBlock()->SectorsPerCluster;
    char buf[sectorSize];

    std::fstream newFile;
    auto mode = std::ios_base::in | std::ios_base::out | std::ios_base::binary;

    newFile.open(srcPath, mode);
    RIF_MF(m_file.good(), "failed to open file '%s'\n", srcPath.c_str());
    
    // TODO: overwrite file if it exists
    // TODO: support nested directories
    // TODO: support long file names

    DirectoryEntry *dirEntry = nullptr;
    for (int i = 0; i < GetParamBlock()->MaxRootDirEntries; i++) {
        char c = m_rootDir[i].Name[0];
        if (c == 0x00 || c == 0xE5) {
            dirEntry = &m_rootDir[i];
            break;
        }
    }
    RIF_M(dirEntry, "root directory is full\n");

    std::string filename = get_filename(srcPath);
    std::string basename = get_basename(filename);
    std::string extension = get_extension(filename);

    // Handle dotfiles
    if (basename.empty() && !extension.empty()) {
        basename = extension;
        extension = "";
    }

    std::string shortName = ConvertToShortName(basename);
    std::string shortExt = ConvertToShortExtension(extension);

    if (shortName.empty() && shortExt.empty()) {
        ERRORF("invalid file name '%s'\n", filename.c_str());
        return false;
    }

    SetString(dirEntry->Name, shortName, FILENAME_LENGTH);
    SetString(dirEntry->Extension, shortExt, EXTENSION_LENGTH);

    time_t now = time(0);
    SetDate(dirEntry->CreationDate, now);
    SetTime(dirEntry->CreationTime, now);
    SetDate(dirEntry->ModifiedDate, now);
    SetTime(dirEntry->ModifiedTime, now);
    SetDate(dirEntry->LastAccessDate, now);

    dirEntry->Attributes = ATTR_AR;
    dirEntry->_Reserved1 = 0;
    dirEntry->_Reserved2 = 0;
    dirEntry->_Reserved3 = 0;

    newFile.seekg(0, std::ios_base::end);
    dirEntry->FileSize = newFile.tellg();
    newFile.seekg(0, std::ios_base::beg);

    int firstCluster = 0;
    int lastCluster = -1;
    int currCluster = 0;
    int numClusters = 0;

    int bytesRemaining = dirEntry->FileSize;

    // TODO: alloc table/disk space bounds check

    while (bytesRemaining > 0) {
        while (GetClusterTableValue(currCluster) != 0) {
            currCluster++;
        }
        if (lastCluster > 0) {
            SetClusterTableValue(lastCluster, currCluster);
        }
        if (firstCluster < 0) {
            firstCluster = currCluster;
        }

        memset(buf, 0, sectorSize);
        newFile.read(buf, std::min(sectorSize, bytesRemaining));
        RIF_MF(newFile.good(), "failed to read '%s'\n", filename.c_str());

        bytesRemaining -= newFile.gcount();
        WriteDataCluster(currCluster, buf);
        numClusters++;

        lastCluster = currCluster;
        currCluster++;
    }

    if (lastCluster > 2) {
        SetClusterTableValue(lastCluster, GetEndOfClusterChainMarker());
    }
    dirEntry->FirstCluster = firstCluster;

    int clustersInUse = ceil(dirEntry->FileSize, clusterSize);
    PrintInfo("%s: size = %d, size on disk = %d, clusters = %d\n",
        GetShortFileName(dirEntry).c_str(),
        dirEntry->FileSize,
        clustersInUse * clusterSize,
        clustersInUse);

    m_allocTableNeedsUpdate = true;
    m_rootDirNeedsUpdate = true;
    return true;
}

bool FatImage::AddDirectory(const std::string &path)
{
    char parentClustBuf[GetClusterSize()];   
    std::string basePath = get_directory(path);
    std::string newDirName = get_filename(path);

    int numEntries = 0;
    DirectoryEntry *parent = nullptr;

    int parentClust = FindDirectory(path);
    if (parentClust == -1) {
        return false;
    }
    else if (parentClust == 0) {
        parent = m_rootDir;
        numEntries = GetParamBlock()->MaxRootDirEntries;
    }
    else {
        RIF(ReadDataCluster(parentClust, parentClustBuf));
        parent = (DirectoryEntry *) parentClustBuf;
        numEntries = GetClusterSize() / sizeof(DirectoryEntry);
    }

    // TODO: check for existing
    bool foundSlot = false;
    DirectoryEntry *newDirEntry = nullptr;
    for (int i = 0; i < numEntries; i++) {
        char c = parent[i].Name[0];
        if (c == 0x00 || c == 0xE5) {
            newDirEntry = &parent[i];
            foundSlot = true;
            break;
        }
    }

    // TODO: add directory cluster if current one is full (and it's not the root)
    RIT_MF(!foundSlot /*&& cluster == 0*/, "'%s' is full!\n", basePath.empty() ? "/" : basePath.c_str());

    InitDirEntry(newDirEntry, newDirName);
    int nextFree = FindNextFreeCluster();
    if (nextFree == -1) {
        ERROR("disk is full!\n");
        return false;
    }
    newDirEntry->FirstCluster = nextFree;

    char clustBuf[GetClusterSize()] = { 0 };
    DirectoryEntry *dotEntry = (DirectoryEntry *) &clustBuf;
    dotEntry[0] = *newDirEntry;
    SetString(dotEntry[0].Name, ".", FILENAME_LENGTH);
    SetString(dotEntry[0].Extension, "", EXTENSION_LENGTH);

    if (parent != m_rootDir) {
        dotEntry[1] = *parent;
    }
    else {
        dotEntry[1].Attributes = ATTR_DIR;
    }
    SetString(dotEntry[1].Name, "..", FILENAME_LENGTH);
    SetString(dotEntry[1].Extension, "", EXTENSION_LENGTH);

    RIF(WriteDataCluster(nextFree, clustBuf));
    if (parentClust != 0) {
        RIF(WriteDataCluster(parentClust, parentClustBuf));
    }
    SetClusterTableValue(nextFree, GetEndOfClusterChainMarker());
    
    m_rootDirNeedsUpdate = true;
    m_allocTableNeedsUpdate = true;
    return true;
}

void FatImage::InitDirEntry(DirectoryEntry *dirEntry, const std::string &name)
{
    SetString(dirEntry->Name, ConvertToShortName(name), FILENAME_LENGTH);
    SetString(dirEntry->Extension, "", EXTENSION_LENGTH);

    time_t now = time(0);
    SetDate(dirEntry->CreationDate, now);
    SetTime(dirEntry->CreationTime, now);
    SetDate(dirEntry->ModifiedDate, now);
    SetTime(dirEntry->ModifiedTime, now);
    SetDate(dirEntry->LastAccessDate, now);

    dirEntry->Attributes = ATTR_DIR;
    dirEntry->FileSize = 0;
    dirEntry->FirstCluster = 0;
    dirEntry->_Reserved1 = 0;
    dirEntry->_Reserved2 = 0;
    dirEntry->_Reserved3 = 0;
}

int FatImage::FindDirectory(const std::string &path)
{
    char clustBuf[GetClusterSize()];
    int curClust = 0;
    int curDirSize = GetParamBlock()->MaxRootDirEntries;
    std::string curDirName;
    DirectoryEntry *curDir = m_rootDir;

    std::string s = path;
    size_t sep = 0;

    while ((sep = s.find_first_of("/\\")) != std::string::npos) {
        curDirName = ConvertToShortName(s.substr(0, sep));
        s.erase(0, sep + 1);
        
    SearchDir:
        bool found = false;
        for (int i = 0; i < curDirSize; i++) {
            if ((curDir[i].Attributes & ATTR_DIR) != ATTR_DIR) {
                continue;
            }
            std::string name = GetString(curDir[i].Name, FILENAME_LENGTH);
            if (name.compare(curDirName) != 0) {
                continue;   
            }

            found = true;
            curClust = curDir[i].FirstCluster;
            if (!ReadDataCluster(curClust, clustBuf)) {
                return -1;
            }
            curDir = (DirectoryEntry *) clustBuf;
            curDirSize = GetClusterSize() / sizeof(DirectoryEntry);
            break;
        }

        if (!found) {
            if (curClust == 0) goto NotFound;
            
            int nextClust = GetClusterTableValue(curClust);
            if (nextClust == GetEndOfClusterChainMarker()) {
                goto NotFound;
            }
            curClust = nextClust;
            if (!ReadDataCluster(curClust, clustBuf)) {
                return -1;
            }
            curDir = (DirectoryEntry *) clustBuf;
            curDirSize = GetClusterSize() / sizeof(DirectoryEntry);
            goto SearchDir;
        }
    }

    return curClust;

NotFound:
    ERRORF("directory not found '%s'\n", curDirName.c_str());
    return -1;
}

void FatImage::CreateBootSector()
{
    int cyl = 80;                                   // TODO: command-line param for this
    int hed = 2;                                    // TODO: command-line param for this
    int sec = 18;                                   // TODO: command-line param for this

    if (m_bootSect) {
        delete m_bootSect;
    }

    m_bootSect = new BootSector();
    SetOemName("fatfs");                            // TODO: command-line param for this
    SetVolumeLabel("NO NAME");                      // TODO: command-line param for this
    SetFileSystemType("FAT12");
    GetParamBlock()->VolumeId = (uint32_t) time(0); // TODO: command-line param for this
    GetParamBlock()->DriveNumber = 0;               // TODO: command-line param for this
    GetParamBlock()->MediaType = 0xF0;              // 3.5in floppy, TODO: command-line param for this
    GetParamBlock()->HeadCount = hed;
    GetParamBlock()->SectorsPerTrack = sec;
    GetParamBlock()->SectorCount = cyl * hed * sec;
    GetParamBlock()->SectorSize = 512;              // TODO: command-line param for this
    GetParamBlock()->SectorsPerCluster = 1;         // TODO: command-line param for this
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

    
    char bootcode[BOOTCODE_SIZE] = {                // TODO: command-line param for this
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
    RIF_M(m_file.good(), "failed to read boot sector\n");

    return true;
}

bool FatImage::WriteBootSector()
{
    if (!m_bootSect) {
        return false;
    }

    m_file.seekp(0, std::ios_base::beg);
    m_file.write((char *) m_bootSect, sizeof(BootSector));
    RIF_M(m_file.good(), "failed to write boot sector\n");

    return true;
}

void FatImage::CreateFileAllocTable()
{
    size_t fatSize = GetParamBlock()->SectorsPerTable * GetParamBlock()->SectorSize;
    m_allocTable = new char[fatSize]();

    SetClusterTableValue(0, 0xFF0); // Reserved cluster: ??? (endianness marker?)
    SetClusterTableValue(1, 0xFFF); // Reserved cluster: End-of-chain marker value
}

bool FatImage::LoadFileAllocTable()
{
    if (m_allocTable) {
        delete[] m_allocTable;
    }

    size_t fatSize = GetParamBlock()->SectorsPerTable * GetParamBlock()->SectorSize;
    m_allocTable = new char[fatSize];

    for (int i = 0; i < GetParamBlock()->TableCount; i++) {
        // Read-in all FATs, keep only the last one
        m_file.read((char *) m_allocTable, fatSize);
        RIF_M(m_file.good(), "failed to create file allocation table\n");
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

    m_file.seekp(seekOffset, std::ios_base::beg);
    for (int i = 0; i < GetParamBlock()->TableCount; i++) {
        m_file.write((char *) m_allocTable, allocTableSize);
        RIF_M(m_file.good(), "failed to write file allocation table\n");
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
    RIF_M(m_file.good(), "failed to read root directory\n");

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

    m_file.seekp(seekOffset, std::ios_base::beg);
    m_file.write((char *) m_rootDir, rootDirSize);
    RIF_M(m_file.good(), "failed to write root directory\n");

    return true;
}

bool FatImage::ReadDataCluster(int num, char *data)
{
    int sectorSize = GetParamBlock()->SectorSize;
    int sectorsPerCluster = GetParamBlock()->SectorsPerCluster;
    int clusterSize = sectorSize * sectorsPerCluster;

    RIF_MF(num >= 2, "cannot read reserved cluster %d\n", num);
    RIF_MF(num < GetClusterCount() + 2, "invalid cluser %d\n", num);
    int actualNum = num - 2;    // Clusters 0 and 1 are reserved, data clusters actually begin at 2

    long seekOffset = (GetFirstDataSectorNumber() * sectorSize) + (actualNum * clusterSize);
    m_file.seekp(seekOffset, std::ios_base::beg);
    m_file.read(data, clusterSize);
    RIF_MF(m_file.good(), "failed to read cluster %d\n", num);

    return true;
}

bool FatImage::WriteDataCluster(int num, char *data)
{
    int sectorSize = GetParamBlock()->SectorSize;
    int sectorsPerCluster = GetParamBlock()->SectorsPerCluster;
    int clusterSize = sectorSize * sectorsPerCluster;

    RIF_MF(num >= 2, "cannot write reserved cluster %d\n", num);
    RIF_MF(num < GetClusterCount() + 2, "invalid cluser %d\n", num);
    int actualNum = num - 2;    // Clusters 0 and 1 are reserved, data clusters actually begin at 2

    long seekOffset = (GetFirstDataSectorNumber() * sectorSize) + (actualNum * clusterSize);
    m_file.seekp(seekOffset, std::ios_base::beg);
    m_file.write(data, clusterSize);
    RIF_MF(m_file.good(), "failed to write cluster %d\n", num);

    return true;
}

uint16_t FatImage::GetClusterTableValue(int num) const
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

void FatImage::SetClusterTableValue(int num, uint16_t value)
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

int FatImage::FindNextFreeCluster() const
{
    for (int i = 2; i < GetClusterCount(); i++) {
        if (GetClusterTableValue(i) == 0) {
            return i;
        }
    }
    return -1;
}

uint16_t FatImage::GetEndOfClusterChainMarker() const
{
    return GetClusterTableValue(1);
}

bool FatImage::ZeroData()
{
    int sectorSize = GetParamBlock()->SectorSize;
    char zeroBuf[sectorSize] = { 0 };

    int firstDataSector = GetFirstDataSectorNumber();
    long seekOffset = firstDataSector * sectorSize;
    m_file.seekp(seekOffset, std::ios_base::beg);

    for (int i = firstDataSector; i < GetParamBlock()->SectorCount; i++) {
        m_file.write(zeroBuf, sectorSize);
        RIF_MF(m_file.good(), "failed to write cluster %d\n", i);
    }

    return true;
}

std::string FatImage::ConvertToShortName(const std::string &basename) const
{
    std::string s = trim(upper(basename));
    s.erase(std::remove_if(s.begin(), s.end(), FatImage::IsValidShortNameChar), s.end());
    if (s.length() > FILENAME_LENGTH) {
        s = s.substr(0, 6) + "~1";
    }
    return s;
}

std::string FatImage::ConvertToShortExtension(const std::string &extension) const
{
    std::string s = trim(upper(extension));
    s.erase(std::remove_if(s.begin(), s.end(), IsValidShortNameChar), s.end());
    return s.substr(0, EXTENSION_LENGTH);
}

bool FatImage::IsValidShortNameChar(char c)
{
    static const std::string BadChars(
        "\x00\x01\x02\x03\x04\x05\x06\x07"
        "\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
        "\x10\x11\x12\x13\x14\x15\x16\x17"
        "\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"
        "\x7F\xE5\"\\*/:<>?|+,.;=[]", 50);
    return BadChars.find(c) != std::string::npos;
}

std::string FatImage::GetShortFileName(const DirectoryEntry *dirEntry) const
{
    std::string name = trim(GetString(dirEntry->Name, FILENAME_LENGTH));
    std::string ext = trim(GetString(dirEntry->Extension, EXTENSION_LENGTH));

    std::string shortName = name;
    if (!ext.empty()) {
        name += "." + ext;
    }

    return name;
}

// std::string FatImage::GetLongFileName(const DirectoryEntry *dirEntry) const
// {
//     // TODO
//     return "";
// }

void FatImage::SetDate(FatDate &date, time_t t)
{
    tm *ltm = localtime(&t);
    date.Year = ltm->tm_year - 80;    // FAT year0 = 1980, tm year0 = 1900
    date.Month = ltm->tm_mon + 1;     // FAT month: 1-12
    date.Day = ltm->tm_mday ;         // FAT day: 1-31
}

void FatImage::SetTime(FatTime &time, time_t t)
{
    tm *ltm = localtime(&t);
    time.Hours = ltm->tm_hour;        // FAT hour: 0-23
    time.Minutes = ltm->tm_min;       // FAT min: 0-59
    time.Seconds = ltm->tm_sec / 2;   // FAT sec: 0-29 (secs / 2)
}

std::string FatImage::GetString(const char *src, int length) const
{
    std::string s(src, length);
    return rtrim(s);
}

void FatImage::SetString(char *dest, const std::string &src, int length)
{
    std::string s = src;
    s.resize(length, ' ');
    strncpy(dest, s.c_str(), length);
}

void FatImage::PrintInfo(const char *fmt, ...) const
{
    va_list args;
    va_start(args, fmt);
    
    if (!Quiet) vprintf(fmt, args);
    
    va_end(args);
}
