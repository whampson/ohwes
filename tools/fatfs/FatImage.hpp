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
 *    File: tools/fatfs/FatImage.hpp                                          *
 * Created: November 29, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#ifndef __FATIMAGE_HPP
#define __FATIMAGE_HPP

#include <fstream>
#include <string>
#include "fat.hpp"

class FatImage
{
public:
    bool Quiet;

    FatImage();
    ~FatImage();

    bool Create(const std::string &path);
    bool Load(const std::string &path);

    bool AddFile(const std::string &srcPath);
    
    std::string GetVolumeLabel() const;
    void SetVolumeLabel(const std::string &label);

    std::string GetOemName() const;
    void SetOemName(const std::string &name);

    std::string GetFileSystemType() const;
    void SetFileSystemType(const std::string &name);

    BiosParameterBlock * GetParamBlock() const;

private:
    std::fstream m_file;
    BootSector *m_bootSect;
    DirectoryEntry *m_rootDir;
    char *m_allocTable;
    bool m_bootSectNeedsUpdate;
    bool m_allocTableNeedsUpdate;
    bool m_rootDirNeedsUpdate;

    int GetFirstFileAllocSectorNumber() const;
    int GetFirstRootDirSectorNumber() const;
    int GetFirstDataSectorNumber() const;

    void CreateBootSector();
    void CreateFileAllocTable();
    void CreateRootDirectory();

    bool LoadBootSector();
    bool LoadFileAllocTable();
    bool LoadRootDirectory();

    bool WriteBootSector();
    bool WriteFileAllocTable();
    bool WriteRootDirectory();

    uint16_t GetClusterTableValue(int num) const;
    void SetClusterTableValue(int num, uint16_t value);

    bool WriteDataCluster(int num, char *data);
    bool ZeroData();

    std::string ConvertToShortName(const std::string &basename) const;
    std::string ConvertToShortExtension(const std::string &extension) const;

    std::string GetShortFileName(const DirectoryEntry *dirEntry) const;
    std::string GetLongFileName(const DirectoryEntry *dirEntry) const;

    std::string GetString(const char *src, int length) const;
    void SetString(char *dest, const std::string &src, int length);
    
    void PrintInfo(const char *fmt, ...) const;

    static bool IsValidShortNameChar(char c);
};

#endif  // __FATIMAGE_HPP
