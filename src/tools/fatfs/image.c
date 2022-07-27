#include "image.h"
#include "fatfs.h"
#include "fat12.h"

static BootSector s_BootSect;
static uint32_t *s_pClusterMap = NULL;
static uint32_t s_NumClusters = 0;
static DirectoryEntry *s_pRootDir = NULL;
static char s_ImagePath[MAX_PATH];

static size_t GetClusterAddress(uint32_t cluster);

bool OpenImage(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
    {
        LogError("could not open disk image file\n");
        return false;
    }

    bool success = true;

    // Load boot sector
    fread(&s_BootSect, 1, sizeof(BootSector), fp);
    if (ferror(fp))
    {
        LogError("could not read boot sector\n");
        success = false;
        goto Cleanup;
    }

    int sectorSize = GetBiosParams()->SectorSize;
    int resSectors = GetBiosParams()->ReservedSectorCount;
    int fatSectors = GetBiosParams()->SectorsPerTable;
    int numFats = GetBiosParams()->TableCount;

    // Skip remaining reserved sectors
    // Boot sector is considered reserved
    fseek(fp, sectorSize * (resSectors - 1), SEEK_CUR);

    // TODO: detect FAT type
    // For now, assume FAT12...

    int fatSize = fatSectors * sectorSize;
    int clusterCount = (fatSize / 3) * 2;   // 3 bytes = 24 bits = 2 FAT12 clusters
    SafeAlloc(s_pClusterMap, clusterCount * sizeof(uint32_t));
    s_NumClusters = clusterCount;

    int pairCount = clusterCount / 2;
    int bytesRead = 0;
    for (int i = 0; i < pairCount; i++)
    {
        uint8_t stride[3];
        bytesRead += fread(stride, 1, sizeof(stride), fp);
        if (ferror(fp))
        {
            LogError("could not read cluster map\n");
            success = false;
            goto Cleanup;
        }

        // |........|++++....|++++++++|

        uint32_t cluster0 = (stride[1] & 0xF) << 8 | stride[0];
        uint32_t cluster1 = (stride[2] << 4) | (stride[1] >> 4);

        s_pClusterMap[i * 2] = cluster0;
        s_pClusterMap[i * 2 + 1] = cluster1;
    }

    // Skip other copies of the FAT
    // TODO: error detection/correction?
    fseek(fp, sectorSize * fatSectors * (numFats - 1), SEEK_CUR);

    int numDirEntries = GetBiosParams()->MaxRootDirEntryCount;
    SafeAlloc(s_pRootDir, numDirEntries * sizeof(DirectoryEntry));

    fread(s_pRootDir, 1, numDirEntries * sizeof(DirectoryEntry), fp);
    if (ferror(fp))
    {
        LogError("could not read root directory\n");
        success = false;
        goto Cleanup;
    }

    if (success)
    {
        strncpy(s_ImagePath, path, MAX_PATH);
    }

Cleanup:
    if (!success)
    {
        SafeFree(s_pClusterMap);
        SafeFree(s_pRootDir);
    }
    fclose(fp);
    return success;
}

void CloseImage(void)
{
    SafeFree(s_pRootDir);
    SafeFree(s_pClusterMap);
    s_ImagePath[0] = '\0';
}

bool IsImageOpen(void)
{
    return s_pClusterMap != NULL;
}

const char * GetImagePath(void)
{
    return s_ImagePath;
}

BootSector * GetBootSector(void)
{
    return &s_BootSect;
}

BiosParamBlock * GetBiosParams(void)
{
    return &s_BootSect.BiosParams;
}

uint32_t GetNextCluster(uint32_t cluster)
{
    if (cluster >= s_NumClusters)
    {
        return CLUSTER_END;
    }

    if (cluster == CLUSTER_END)
    {
        return CLUSTER_END;
    }
    // TODO: bad clusters?

    return s_pClusterMap[cluster];
}

const DirectoryEntry * FindFile(const char *path)
{
    char realPath[MAX_PATH];
    char *fileName;

    strncpy(realPath, path, MAX_PATH);
    Uppercase(realPath);

    // TODO: walk path
    fileName = realPath;

    int index = 0;
    int count = GetBiosParams()->MaxRootDirEntryCount;

    char requestedName[NAME_LENGTH + 1] = { 0 };
    char requestedExt[EXTENSION_LENGTH + 1] = { 0 };

    char tmpName[NAME_LENGTH + 1] = { 0 };
    char tmpExt[EXTENSION_LENGTH + 1] = { 0 };

    char *tok = strtok(fileName, ".");
    if (tok != NULL)
    {
        strncpy(requestedName, tok, NAME_LENGTH);
    }

    tok = strtok(NULL, ".");
    if (tok != NULL)
    {
        strncpy(requestedExt, tok, EXTENSION_LENGTH);
    }

    bool found = false;
    while (!found && index < count)
    {
        const DirectoryEntry *e = &s_pRootDir[index++];
        switch ((unsigned char) e->Name[0])
        {
            case 0x00:
            case 0x05:
            case 0xE5:
                // free slot/deleted file
                continue;
        }
        GetName(tmpName, e->Name);
        GetExt(tmpExt, e->Extension);

        bool nameMatch = strcmp(tmpName, requestedName) == 0;
        bool extMatch = strcmp(tmpExt, requestedExt) == 0;
        if (nameMatch && extMatch)
        {
            return e;
        }
    }

    return NULL;
}

bool ReadFile(const DirectoryEntry *entry, char *buf)
{
    const BiosParamBlock *bpb = GetBiosParams();

    FILE *fp = fopen(s_ImagePath, "rb");
    if (!fp)
    {
        LogError("unable to read disk image\n");
        return false;
    }

    uint32_t cluster = entry->FirstCluster;
    size_t fileSize = entry->FileSize;
    size_t clusterSize = bpb->SectorSize * bpb->SectorsPerCluster;
    size_t bytesRead = 0;
    size_t totalBytesRead = 0;
    size_t offset = 0;
    size_t readSize;

    bool success = true;
    do
    {
        offset = GetClusterAddress(cluster);
        fseek(fp, offset, SEEK_SET);

        readSize = min(clusterSize, (fileSize - totalBytesRead));

        bytesRead = fread(buf, 1, readSize, fp);
        if (ferror(fp))
        {
            LogError("error reading disk image\n");
            success = false;
            goto Cleanup;
        }
        buf += bytesRead;
        totalBytesRead += bytesRead;
        cluster = GetNextCluster(cluster);
    } while (cluster != CLUSTER_END);

Cleanup:
    fclose(fp);
    return success;
}

static size_t GetClusterAddress(uint32_t cluster)
{
    const BiosParamBlock *bpb = GetBiosParams();

    size_t firstClusterAddress =
        (bpb->ReservedSectorCount + (bpb->TableCount * bpb->SectorsPerTable))
        * bpb->SectorSize
        + (bpb->MaxRootDirEntryCount * sizeof(DirectoryEntry));

    return firstClusterAddress + ((cluster - 2) * bpb->SectorSize);
}
