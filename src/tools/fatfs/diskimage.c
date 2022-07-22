#include "diskimage.h"
#include "fatfs.h"
#include "fat12.h"

static BootSector g_BootSect;
static uint16_t *g_pClusterMap = NULL;
static DirectoryEntry *g_pRootDir = NULL;
static char g_FilePath[MAX_PATH];

static int GetClusterOffset(int clusterNum);

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

    if (success)
    {
        strncpy(g_FilePath, path, MAX_PATH);
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


const DirectoryEntry * FindFile(const char *path)
{
    char realPath[MAX_PATH];
    char *fileName;
    strncpy(realPath, path, MAX_PATH);

    Uppercase(realPath);
    fileName = realPath;

    // TODO: walk path

    bool found = false;

    int index = 0;
    int count = g_BootSect.BiosParams.MaxRootDirEntryCount;

    const char *const Delimiter = ".";

    char reqName[NAME_LENGTH + 1] = { 0 };
    char reqExt[EXTENSION_LENGTH + 1] = { 0 };

    char name[NAME_LENGTH + 1] = { 0 };
    char ext[EXTENSION_LENGTH + 1] = { 0 };

    char *tok = strtok(fileName, Delimiter);
    if (tok != NULL)
    {
        strcpy(reqName, tok);
    }

    tok = strtok(NULL, Delimiter);
    if (tok != NULL)
    {
        strcpy(reqExt, tok);
    }

    while (!found && index < count)
    {
        const DirectoryEntry *e = &g_pRootDir[index++];
        strncpy(name, e->Name, NAME_LENGTH);
        strncpy(ext, e->Extension, EXTENSION_LENGTH);

        int nameEnd = -1;
        int extEnd = -1;
        for (int i = 0; i < NAME_LENGTH; i++)
        {
            if (nameEnd == -1 && name[i] == ' ')
            {
                nameEnd = i;
                name[i] = '\0';
            }

            if (i < EXTENSION_LENGTH && extEnd == -1 && ext[i] == ' ')
            {
                extEnd = i;
                ext[i] = '\0';

            }
        }

        bool nameMatch = strcmp(name, reqName) == 0;
        bool extMatch = strcmp(ext, reqExt) == 0;
        if (nameMatch && extMatch)
        {
            return e;
        }
    }

    return NULL;
}

bool ReadFile(const DirectoryEntry *entry, char *buf)
{
    const BiosParamBlock *bpb = &g_BootSect.BiosParams;

    int nextCluster = entry->FirstCluster;

    FILE *fp = fopen(g_FilePath, "rb");
    if (!fp)
    {
        LogError("unable to read disk image\n");
        return 2;
    }

    int bytesRead = 0;
    int offset;

    do
    {
        offset = GetClusterOffset(entry->FirstCluster);
        fseek(fp, offset, SEEK_SET);

        bytesRead += fread(buf, 1, bpb->SectorSize, fp);
        nextCluster = g_pClusterMap[nextCluster];
    } while (nextCluster != CLUSTER_END);

    fclose(fp);

    return bytesRead;
}

void PrintDiskInfo()
{
    printf("TODO\n");
}

static int GetClusterOffset(int clusterNum)
{
    const BiosParamBlock *bpb = &g_BootSect.BiosParams;

    const int FirstClusterOffset =
        (bpb->ReservedSectorCount + (bpb->TableCount * bpb->SectorsPerTable))
        * bpb->SectorSize
        + (bpb->MaxRootDirEntryCount * sizeof(DirectoryEntry));

    return FirstClusterOffset + ((clusterNum - 2) * bpb->SectorSize);
}
