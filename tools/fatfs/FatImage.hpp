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
    FatImage();
    ~FatImage();

    bool Create(const std::string &filename);
    bool Load(const std::string &filename);
    
    std::string GetVolumeLabel() const;
    void SetVolumeLabel(const std::string &label);

    std::string GetOemName() const;
    void SetOemName(const std::string &name);

    std::string GetFileSystemType() const;
    void SetFileSystemType(const std::string &name);

    int GetDataStartSector() const;
    BiosParameterBlock * GetParamBlock() const;

private:
    bool m_isFileOpen;
    std::fstream m_file;
    BootSector *m_bootSect;
    DirectoryEntry *m_rootDir;
    char *m_allocTable;
    bool m_bootSectNeedsUpdate;
    bool m_allocTableNeedsUpdate;
    bool m_rootDirNeedsUpdate;

    void CreateBootSector();
    void CreateFileAllocTable();
    void CreateRootDirectory();

    bool LoadBootSector();
    bool LoadFileAllocTable();
    bool LoadRootDirectory();

    bool WriteBootSector();
    bool WriteFileAllocTable();
    bool WriteRootDirectory();

    // error

    void WriteString(char *dest, const std::string &source, int length);
};

#endif  // __FATIMAGE_HPP
