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
#include <getopt.h>
#include "helpers.hpp"
#include "fat.hpp"
#include "FatImage.hpp"

#define E_ARG                   1
#define E_IO                    2

#define RIF_ARG_M(x,m)          do { if (!(x)) { ERROR(m); return E_ARG; } } while(0)
#define RIF_ARG_MF(x,m,...)     do { if (!(x)) { ERRORF(m, __VA_ARGS__); return E_ARG; } } while(0)
#define RIT_ARG_M(x,m)          do { if (x) { ERROR(m); return E_ARG; } } while(0)
#define RIT_ARG_MF(x,m,...)     do { if (x) { ERRORF(m, __VA_ARGS__); return E_ARG; } } while(0)
#define RIF_IO(x)               do { if (!(x)) return E_IO; } while(0)

struct command {
    const char *name;
    int (*func)(int, char **);
};

static int add(int argc, char **argv);
static int create(int argc, char **argv);
static int info(int argc, char **argv);
static int mkdir(int argc, char **argv);
static int help(int argc, char **argv);

static struct command cmds[] = {
    { "add", add },
    { "create", create },
    { "info", info },
    { "mkdir", mkdir },
    { "help", help },
    { NULL, NULL },
};

static const char *image_file = NULL;
static FatImage fs_image;

int main(int argc, char **argv)
{
    int opt_help = 0;
    struct option long_options[] = {
        { "help", no_argument, &opt_help, 1 },
        { "image", required_argument, NULL, 'i' },
        { "quiet", no_argument, NULL, 'q' }
    };

    int o;
    opterr = 0;
    while (1) {
        int option_index = 0;
        o = getopt_long(argc, argv, "i:q", long_options, &option_index);
        if (o == -1) break;

        switch (o) {
        case 'i':
            image_file = optarg;
            break;
        case 'q':
            fs_image.Quiet = true;
            break;
        case '?':
            RIT_ARG_M(optopt == 'i', "missing disk image\n");
            RIT_ARG_M(optopt = 0, "invalid argument\n");
            RIT_ARG_MF(isprint(optopt), "unknown option '%c'\n", optopt);
            ERRORF("unknown option character '\\x%02X'\n", optopt);
            return E_ARG;
        }
    }

    if (opt_help) {
        return help(0, NULL);
    }

    RIT_ARG_M(optind >= argc, "missing command\n");
    
    const char *cmd = argv[optind];
    int cmdargc = argc - optind - 1;
    char **cmdargv = argv + optind + 1;

    int i = 0;
    while (cmds[i].name != NULL) {
        if (strcmp(cmd, cmds[i].name) == 0) {
            return cmds[i].func(cmdargc, cmdargv);
        }
        i++;
    }

    ERRORF("invalid command '%s'\n", cmd);
    return E_ARG;
}

static int add(int argc, char **argv)
{
    RIF_ARG_M(argc > 0, "missing file\n");
    RIF_ARG_M(image_file, "missing disk image\n");

    RIF_IO(fs_image.Load(image_file));
    RIF_IO(fs_image.AddFile(argv[0]));

    return 0;
}

static int create(int argc, char **argv)
{
    RIF_ARG_M(image_file, "missing disk image\n");
    RIF_IO(fs_image.Create(image_file));
    return 0;
}

static int info(int argc, char **argv)
{
    // printf("Volume Label:        %s\n", fs.GetVolumeLabel().c_str());
    // printf("Volume ID:           %08X\n", fs.GetParamBlock()->VolumeId);
    // printf("Media Type:          %Xh\n", fs.GetParamBlock()->MediaType);
    // printf("Drive Number:        %d\n", fs.GetParamBlock()->DriveNumber);
    // printf("FAT Count:           %d\n", fs.GetParamBlock()->TableCount);
    // printf("Head Count:          %d\n", fs.GetParamBlock()->HeadCount);
    // printf("Sector Size:         %d\n", fs.GetParamBlock()->SectorSize);
    // printf("Sector Count:        %d\n", fs.GetParamBlock()->SectorCount);
    // printf("Sectors per Track:   %d\n", fs.GetParamBlock()->SectorsPerTrack);
    // printf("Sectors per FAT:     %d\n", fs.GetParamBlock()->SectorsPerTable);
    // printf("Sectors per Cluster: %d\n", fs.GetParamBlock()->SectorsPerCluster);
    // printf("Reserved Sectors:    %d\n", fs.GetParamBlock()->ReservedSectorCount);
    // printf("Hidden Sectors:      %d\n", fs.GetParamBlock()->HiddenSectorCount);
    // printf("Large Sectors:       %d\n", fs.GetParamBlock()->LargeSectorCount);
    // printf("Max Root Dir Size:   %d\n", fs.GetParamBlock()->MaxRootDirEntries);
    // printf("Extended Boot Sig:   %Xh\n", fs.GetParamBlock()->ExtendedBootSignature);
    // printf("File System Type:    %s\n", fs.GetFileSystemType().c_str());
    // printf("OEM Name:            %s\n", fs.GetOemName().c_str());

    return 0;
}

static int mkdir(int argc, char **argv)
{
    RIT_ARG_M(argc == 0, "missing directory name\n");
    
    RIF_IO(fs_image.Load(image_file));
    RIF_IO(fs_image.AddDirectory(argv[0]));
    
    return 0;
}

static int help(int argc, char **argv)
{
    // TODO: not done

    if (argc == 0) {
        printf("fatfs - FAT file system disk image utility\n");
        printf("Usage: fatfs [options] command args\n");
        printf("\n");
        printf("Options:\n");
        printf("    -i, --image     specifies the disk image to work with\n");
        printf("    -q, --quiet     suppreses extraneous output\n");
        printf("\n");
        printf("Commands:\n");
        printf("    add             add a new file or directory\n");
        printf("    create          create a new blank disk image\n");
        // printf("    extract         extract a file or directory\n");
        // printf("    info            gets information about the disk or a file\n");
        // printf("    list            list the files in a directory\n");
        printf("    mkdir           create a new empty directory\n");
        // printf("    remove          remove a file or directory\n");
        // printf("    rename          renames a file or directory\n");
        // printf("    touch           update a file's metadata\n");
        printf("\n");
        printf("Run 'fatfs help command' to get more information about a specific command.\n");
        return 0;
    }

    if (strcmp(argv[0], "add") == 0) {
        printf("Adds a file or directory to the image.\n");
        printf("Usage: fatfs add file [options]\n");
        printf("\n");
        printf("Options:\n");
        printf("    -f, --force     overwrite any existing files with the same name\n");
    }
    if (strcmp(argv[0], "create") == 0) {

    }
    if (strcmp(argv[0], "extract") == 0) {
        
    }
    if (strcmp(argv[0], "info") == 0) {
        
    }
    if (strcmp(argv[0], "mkdir") == 0) {
        
    }
    if (strcmp(argv[0], "remove") == 0) {
        
    }
    if (strcmp(argv[0], "touch") == 0) {
        
    }

    return 0;
}
