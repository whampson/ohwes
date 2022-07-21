#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "fat12.h"

void PrintBootSector(const BootSector *bootsect)
{
    const BiosParamBlock *bpb = &bootsect->BiosParams;

    if (bpb->ExtendedBootSignature == EXT_BOOT_SIG)
    {
        printf("File System = %.8s\n", bpb->FileSystemType);
        printf("Volume Label = %.11s\n", bpb->Label);
        printf("Volume ID = 0x%x\n", bpb->VolumeId);
    }
    else if (bpb->ExtendedBootSignature == EXT_BOOT_SIG_2)
    {
        printf("Volume ID = 0x%x\n", bpb->VolumeId);
    }
    printf("OEM Name = %.8s\n", bootsect->OemName);
    printf("Disk Size = %d\n", bpb->SectorSize * bpb->SectorCount);
    printf("Cluster Size = %d\n", bpb->SectorSize * bpb->SectorsPerCluster);
    printf("Sector Size = %d\n", bpb->SectorSize);
    printf("Sector Count = %d\n", bpb->SectorCount);
    printf("Media Type = 0x%x\n", bpb->MediaType);
    printf("Head Count = %d\n", bpb->HeadCount);
    printf("Sectors per Track = %d\n", bpb->SectorsPerTrack);
    printf("Drive Number = %d\n", bpb->DriveNumber);
    printf("Reserved Sector Count = %d\n", bpb->ReservedSectorCount);
    printf("Hidden Sector Count = %d\n", bpb->HiddenSectorCount);
    printf("Large Sector Count = %d\n", bpb->LargeSectorCount);
    printf("FAT Count = %d\n", bpb->TableCount);
    printf("Sectors per FAT = %d\n", bpb->SectorsPerTable);
    printf("Root Dir Capacity = %d\n", bpb->MaxRootDirEntryCount);
    printf("Extended Boot Signature = 0x%x\n", bpb->ExtendedBootSignature);
}

void PrintRootDirectory(DirectoryEntry *root, int count)
{
    printf("Index of /\n");
    for (int i = 0; i < count; i++)
    {
        DirectoryEntry *e = &root[i];
        unsigned char c = e->Name[0];
        switch (c)
        {
            case 0x00:
                // empty slot
                continue;
            case 0xE5:
                printf("  (deleted)\n");
                continue;
            case 0x2E:
                printf("  (dot)\n");
                continue;
        }

        printf("  %.8s.%.3s\n", e->Name, e->Extension);
    }
}

static BootSector g_Boot;
static DirectoryEntry *g_RootDir = NULL;

bool OpenImage(const char *path)
{
    bool success = true;

    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
    {
        LogError("could not open file\n");
        return false;
    }

    fread(&g_Boot, sizeof(BootSector), 1, fp);
    if (ferror(fp))
    {
        LogError("could not read boot sector\n");
        success = false;
        goto Cleanup;
    }

    // skip FATs for now
    int sectorSize = g_Boot.BiosParams.SectorSize;
    int fatSectors = g_Boot.BiosParams.SectorsPerTable;
    int numFats = g_Boot.BiosParams.TableCount;

    fseek(fp, numFats * (sectorSize * fatSectors), SEEK_CUR);

    int numEntries = g_Boot.BiosParams.MaxRootDirEntryCount;
    g_RootDir = malloc(numEntries * sizeof(DirectoryEntry));
    
    fread(g_RootDir, sizeof(DirectoryEntry), numEntries, fp);
    if (ferror(fp))
    {
        LogError("could not read root directory\n");
        success = false;
        goto Cleanup;
    }

Cleanup:
    fclose(fp);
    return success;    
}

void CloseImage()
{
    if (g_RootDir)
    {
        free(g_RootDir);
        g_RootDir = NULL;
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        LogError("missing file\n");
        return 1;
    }

    OpenImage(argv[1]);

    PrintBootSector(&g_Boot);
    PrintRootDirectory(g_RootDir, g_Boot.BiosParams.MaxRootDirEntryCount);

    CloseImage();
    

    return 0;
}
