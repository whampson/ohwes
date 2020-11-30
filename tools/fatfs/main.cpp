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

#include "fat.hpp"
#include "FatImage.hpp"
#include <string.h>

static void info(FatImage &fs, int argc, char **argv);

int main(int argc, char **argv)
{
    if (argc <= 2) {
        printf("fatfs: error: missing argument\n");
        printf("Usage: fatfs command image [options]\n");
        return 1;
    }

    std::string img = argv[1];
    std::string cmd = argv[2];
    char **args = &argv[3];
    argc -= 2;

    FatImage fs;
    if (cmd.compare("create") == 0) {
        if (!fs.Create(img)) return 2;
    }
    else if (cmd.compare("info") == 0) {
        if (!fs.Load(img)) return 2;
        info(fs, argc, args);
    }
    // else if (command.compare("set-label") == 0) {
    // }
    else {
        printf("fatfs: error: invalid command");
    }

    return 0;
}

static void info(FatImage &fs, int argc, char **argv)
{
    printf("Volume Label:        %s\n", fs.GetVolumeLabel().c_str());
    printf("Volume ID:           %8X\n", fs.GetParamBlock()->VolumeId);
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
}
