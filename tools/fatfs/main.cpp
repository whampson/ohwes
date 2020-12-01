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
 *    File: tools/fatfs/main.cpp                                              *
 * Created: November 28, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#include <cstdio>
#include <cstring>
#include <fstream>
#include "fat.hpp"
#include "FatImage.hpp"

static int add(FatImage &fs, int argc, char **argv);
static int info(FatImage &fs, int argc, char **argv);

int main(int argc, char **argv)
{
    if (argc <= 2) {
        printf("fatfs: error: missing argument\n");
        printf("Usage: fatfs command image [options]\n");
        return 1;
    }

    int c = argc - 3;
    char **v = argv + 3;

    FatImage fs;
    if (strcmp(argv[2], "create") == 0) {
        if (!fs.Create(argv[1])) return 2;
        return 0;
    }

    if (!fs.Load(argv[1])) return 2;
    
    if (strcmp(argv[2], "add") == 0) {
        return add(fs, c, v);
    }
    else if (strcmp(argv[2], "info") == 0) {
        return info(fs, c, v);
    }
    // else if (command.compare("set-label") == 0) {
    // }
    else {
        printf("fatfs: error: invalid command\n");
    }

    return 1;
}

static int add(FatImage &fs, int argc, char **argv)
{
    if (argc == 0) {
        printf("fatfs: error: missing file name\n");
        return 1;
    }

    if (!fs.AddFile(argv[0])) return 1;
    
    return 0;
}

static int info(FatImage &fs, int argc, char **argv)
{
    printf("Volume Label:        %s\n", fs.GetVolumeLabel().c_str());
    printf("Volume ID:           %08X\n", fs.GetParamBlock()->VolumeId);
    printf("Media Type:          %Xh\n", fs.GetParamBlock()->MediaType);
    printf("Drive Number:        %d\n", fs.GetParamBlock()->DriveNumber);
    printf("FAT Count:           %d\n", fs.GetParamBlock()->TableCount);
    printf("Head Count:          %d\n", fs.GetParamBlock()->HeadCount);
    printf("Sector Size:         %d\n", fs.GetParamBlock()->SectorSize);
    printf("Sector Count:        %d\n", fs.GetParamBlock()->SectorCount);
    printf("Sectors per Track:   %d\n", fs.GetParamBlock()->SectorsPerTrack);
    printf("Sectors per FAT:     %d\n", fs.GetParamBlock()->SectorsPerTable);
    printf("Sectors per Cluster: %d\n", fs.GetParamBlock()->SectorsPerCluster);
    printf("Reserved Sectors:    %d\n", fs.GetParamBlock()->ReservedSectorCount);
    printf("Hidden Sectors:      %d\n", fs.GetParamBlock()->HiddenSectorCount);
    printf("Large Sectors:       %d\n", fs.GetParamBlock()->LargeSectorCount);
    printf("Max Root Dir Size:   %d\n", fs.GetParamBlock()->MaxRootDirEntries);
    printf("Extended Boot Sig:   %Xh\n", fs.GetParamBlock()->ExtendedBootSignature);
    printf("File System Type:    %s\n", fs.GetFileSystemType().c_str());
    printf("OEM Name:            %s\n", fs.GetOemName().c_str());

    return 0;
}
