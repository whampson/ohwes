#include "diskimage.h"
#include "fat12.h"

static BootSector g_BootSect;
static uint16_t *g_pClusterMap = NULL;
static DirectoryEntry *g_pRootDir = NULL;

bool OpenImage(const char *path)
{
    bool success = true;

    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
    {
        LogError("could not open file\n");
        return false;
    }

    // Load boot sector
    fread(&g_BootSect, 1, sizeof(BootSector), fp);
    if (ferror(fp))
    {
        LogError("could not read boot sector\n");
        success = false;
        goto Cleanup;
    }

    {
        const BiosParamBlock *bpb = &g_BootSect.BiosParams;

        printf("BOOT SECTOR\n");
        if (bpb->ExtendedBootSignature == EXT_BOOT_SIG)
        {
            printf("  File System               = %.8s\n", bpb->FileSystemType);
            printf("  Volume Label              = %.11s\n", bpb->Label);
            printf("  Volume ID                 = 0x%x\n", bpb->VolumeId);
        }
        else if (bpb->ExtendedBootSignature == EXT_BOOT_SIG_2)
        {
            printf("  Volume ID                 = 0x%x\n", bpb->VolumeId);
        }
        printf("  OEM Name                  = %.8s\n", g_BootSect.OemName);
        printf("  Disk Size                 = %d\n", bpb->SectorSize * bpb->SectorCount);
        printf("  Cluster Size              = %d\n", bpb->SectorSize * bpb->SectorsPerCluster);
        printf("  Sector Size               = %d\n", bpb->SectorSize);
        printf("  Sector Count              = %d\n", bpb->SectorCount);
        printf("  Media Type                = 0x%x\n", bpb->MediaType);
        printf("  Head Count                = %d\n", bpb->HeadCount);
        printf("  Sectors per Track         = %d\n", bpb->SectorsPerTrack);
        printf("  Drive Number              = %d\n", bpb->DriveNumber);
        printf("  Reserved Sector Count     = %d\n", bpb->ReservedSectorCount);
        printf("  Hidden Sector Count       = %d\n", bpb->HiddenSectorCount);
        printf("  Large Sector Count        = %d\n", bpb->LargeSectorCount);
        printf("  FAT Count                 = %d\n", bpb->TableCount);
        printf("  Sectors per FAT           = %d\n", bpb->SectorsPerTable);
        printf("  Root Dir Capacity         = %d\n", bpb->MaxRootDirEntryCount);
        printf("  Ext. Boot Signature       = 0x%x\n", bpb->ExtendedBootSignature);
    }

    int sectorSize = g_BootSect.BiosParams.SectorSize;
    int resSectors = g_BootSect.BiosParams.ReservedSectorCount;
    int fatSectors = g_BootSect.BiosParams.SectorsPerTable;
    int numFats = g_BootSect.BiosParams.TableCount;

    // Skip remaining reserved sectors
    fseek(fp, sectorSize * (resSectors - 1), SEEK_CUR);

    int fatSize = fatSectors * sectorSize;
    int clusterBufferSize = fatSize * 4 / 3;      // scale data size up to 16 bits
    g_pClusterMap = malloc(clusterBufferSize);
    if (!g_pClusterMap)
    {
        LogError("out of memory!");
        success = false;
        goto Cleanup;
    }

    int pairCount = fatSize / 3;
    for (int i = 0; i < pairCount; i++)
    {
        uint8_t pair[3];
        fread(pair, 1, sizeof(pair), fp);
        if (ferror(fp))
        {
            LogError("could not read cluster map\n");
            success = false;
            goto Cleanup;
        }

        uint16_t lo = (pair[1] & 0xF) << 8 | pair[0];
        uint16_t hi = (pair[2] << 4) | (pair[1] >> 4);

        g_pClusterMap[i * 2] = lo;
        g_pClusterMap[i * 2 + 1] = hi;
    }

    // // Skip other copies of the FAT
    // // TODO: error detection/correction?
    fseek(fp, sectorSize * fatSectors * (numFats - 1), SEEK_CUR);

    {
        printf("PARTIAL CLUSTER MAP\n  ");
        for (int i = 0; i < 128; i++)
        {
            if (i != 0 && (i % 8) == 0)
            {
                printf("\n  ");
            }
            printf("0x%03x ", g_pClusterMap[i]);
        }
        printf("\n");
    }

    int numDirEntries = g_BootSect.BiosParams.MaxRootDirEntryCount;
    size_t size = numDirEntries * sizeof(DirectoryEntry);

    g_pRootDir = malloc(size);
    if (!g_pRootDir)
    {
        LogError("out of memory!");
        success = false;
        goto Cleanup;
    }

    fread(g_pRootDir, 1, size, fp);
    if (ferror(fp))
    {
        LogError("could not read root directory\n");
        success = false;
        goto Cleanup;
    }

    {
        printf("INDEX OF /\n");
        for (int i = 0; i < g_BootSect.BiosParams.MaxRootDirEntryCount; i++)
        {
            const DirectoryEntry *e = &g_pRootDir[i];
            unsigned char c = e->Name[0];
            char nameBuf[12] = { 0 };

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

            int n = 0;
            for (int k = 0; k < NAME_LENGTH + EXTENSION_LENGTH; k++)
            {
                c = e->Name[k];
                if (c == ' ') continue;

                if (k == NAME_LENGTH)
                {
                    nameBuf[n] = '.';
                    n++;
                }
                nameBuf[n] = c;
                n++;
            }
            assert(n <= 12);

            printf("  %s\n", nameBuf);
        }
    }

Cleanup:
    fclose(fp);
    return success;    
}

void CloseImage()
{
    SafeFree(g_pRootDir);
    SafeFree(g_pClusterMap);
}
