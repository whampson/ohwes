#include "image.h"
#include "fatfs.h"
#include "fat12.h"

static BootSector s_BootSect;
static uint32_t s_NumClusters = 0;
static uint32_t *s_pClusterMap = NULL;
static DirEntry *s_pRootDir = NULL;
static FILE *s_FilePtr = NULL;
static char s_ImagePath[MAX_PATH];

static size_t GetClusterAddress(uint32_t cluster);
static void MakeUppercase(char *s);

bool OpenImage(const char *path)
{
    bool success = true;

    s_FilePtr = SafeOpen(path, "rb+");

    // Load boot sector
    SafeRead(s_FilePtr, &s_BootSect, sizeof(BootSector));

    int sectorSize = GetBiosParams()->SectorSize;
    int resSectors = GetBiosParams()->ReservedSectorCount;
    int fatSectors = GetBiosParams()->SectorsPerTable;
    int numFats = GetBiosParams()->TableCount;

    // Skip remaining reserved sectors
    // Boot sector is considered reserved
    fseek(s_FilePtr, sectorSize * (resSectors - 1), SEEK_CUR);

    // TODO: detect FAT type
    // For now, assume FAT12...

    int fatSize = fatSectors * sectorSize;
    s_NumClusters = (fatSize / 3) * 2;   // 3 bytes = 24 bits = 2 FAT12 clusters
    s_pClusterMap = SafeAlloc(s_NumClusters * sizeof(uint32_t));

    int pairCount = s_NumClusters / 2;
    int bytesRead = 0;
    for (int i = 0; i < pairCount; i++)
    {
        uint8_t stride[3];
        bytesRead += SafeRead(s_FilePtr, stride, sizeof(stride));

        // |........|++++....|++++++++|
        uint32_t cluster0 = (stride[1] & 0xF) << 8 | stride[0];
        uint32_t cluster1 = (stride[2] << 4) | (stride[1] >> 4);

        s_pClusterMap[i * 2] = cluster0;
        s_pClusterMap[i * 2 + 1] = cluster1;
    }

    // Skip other copies of the FAT
    // TODO: error detection/correction?
    fseek(s_FilePtr, sectorSize * fatSectors * (numFats - 1), SEEK_CUR);

    int numDirEntries = GetBiosParams()->MaxRootDirEntryCount;
    int rootDirSize = numDirEntries * sizeof(DirEntry);

    s_pRootDir = SafeAlloc(rootDirSize);
    SafeRead(s_FilePtr, s_pRootDir, rootDirSize);

    if (success)
    {
        strncpy(s_ImagePath, path, MAX_PATH);
    }

Cleanup:
    if (!success)
    {
        CloseImage();
    }
    return success;
}

void CloseImage(void)
{
    SafeFree(s_pRootDir);
    SafeFree(s_pClusterMap);
    SafeClose(s_FilePtr);
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

DirEntry * GetRootDir(void)
{
    return s_pRootDir;
}

size_t GetClusterSize(void)
{
    return GetBiosParams()->SectorSize * GetBiosParams()->SectorsPerCluster;
}

size_t GetRootDirSize(void)
{
    return GetBiosParams()->MaxRootDirEntryCount * sizeof(DirEntry);
}

size_t GetTableSize(void)
{
    return GetBiosParams()->SectorsPerTable * GetBiosParams()->SectorSize;
}

uint32_t GetNextCluster(uint32_t current)
{
    if (current >= s_NumClusters)
    {
        return CLUSTER_END;
    }

    return s_pClusterMap[current];
}

const DirEntry * FindFileInDir(const DirEntry *dir, const char *fileName)
{
    const size_t ClusterSize = GetClusterSize();

    int count;
    int index;
    bool found = false;
    bool isRoot = (dir == s_pRootDir);
    bool success = false;

    char buf[ClusterSize];

    if (isRoot)
    {
        count = GetBiosParams()->MaxRootDirEntryCount;
    }

    // TODO: long file names
    // TODO: read clusters for non-root dirs
    // TODO: this is fuckin ugly

    char shortName[MAX_SHORTNAME] = { 0 };

    printf("Looking for '%s'...\n", fileName);
    do
    {
        if (!isRoot)
        {
            if (!CLUSTER_IS_VALID(dir->FirstCluster))
            {
                break;
            }
            int addr = GetClusterAddress(dir->FirstCluster);
            SafeRead(s_FilePtr, buf, ClusterSize);
            dir = (const DirEntry *) buf;
            count = ClusterSize / sizeof(DirEntry);
            index = 0;
        }
        while (!found && index < count)
        {
            const DirEntry *e = &dir[index++];
            switch ((unsigned char) e->Name[0])
            {
                case 0x00:
                case 0x05:
                case 0xE5:
                    // free slot/deleted file
                    continue;
            }
            GetShortName(shortName, e);
            printf("    %s\n", shortName);
            if (strcmp(shortName, fileName) == 0)
            {
                dir = e;
                success = true;
                break;
            }
        }
    } while (!isRoot && CLUSTER_IS_VALID(dir->FirstCluster));

Cleanup:
    return (success) ? dir : NULL;
}

const DirEntry * FindFile(const char *path)
{
    char realPath[MAX_PATH];

    strncpy(realPath, path, MAX_PATH);

    int depth = -1;
    char *tok = strtok(realPath, "/");
    const DirEntry *file = s_pRootDir;
    bool found = false;

    while (tok != NULL && file != NULL)
    {
        depth++;

        file = FindFileInDir(file, tok);

        printf("tok = %s\n", tok);
        tok = strtok(NULL, "/");
    }

    printf("depth = %d\n", depth);
    if (file != NULL)
    {
        printf("found name = %.8s\n", file->Name);
        printf("found ext  = %.3s\n", file->Extension);
    }

    return NULL;
}

bool ReadCluster(uint32_t index, char *dst)
{
    const size_t ClusterSize = GetClusterSize();

    if (index >= s_NumClusters ||
        index < CLUSTER_FIRST || index > CLUSTER_LAST)
    {
        LogWarning("attempt to read invalid data cluster 0x%03X\n", index);
        return false;
    }

    bool success = true;
    size_t seekAddr = GetClusterAddress(index);

    fseek(s_FilePtr, seekAddr, SEEK_SET);
    size_t bytesRead = SafeRead(s_FilePtr, dst, ClusterSize);
    assert(bytesRead == ClusterSize);

Cleanup:
    return success;
}

bool ReadFile(const DirEntry *entry, char *dst)
{
    const size_t ClusterSize = GetClusterSize();

    bool success = true;
    uint32_t cluster = entry->FirstCluster;
    size_t fileSize = entry->FileSize;
    size_t totalBytesRead = 0;
    size_t readSize;
    char buf[ClusterSize];

    while (CLUSTER_IS_VALID(cluster))
    {
        readSize = min(ClusterSize, (fileSize - totalBytesRead));

        RIF(ReadCluster(cluster, buf));
        memcpy(dst, buf, readSize);

        dst += readSize;
        totalBytesRead += readSize;
        cluster = GetNextCluster(cluster);
    }

Cleanup:
    return success;
}

static size_t GetClusterAddress(uint32_t cluster)
{
    const BiosParamBlock *bpb = GetBiosParams();

    const size_t NumResSectors = bpb->ReservedSectorCount;
    const size_t NumFatSectors = bpb->TableCount * bpb->SectorsPerTable;
    const size_t SectorSize = bpb->SectorSize;
    const size_t ClusterSize = GetClusterSize();
    const size_t RootDirSize = GetRootDirSize();

    return ((NumResSectors + NumFatSectors) * SectorSize)
        + RootDirSize
        + ((cluster - 2) * ClusterSize);
}

static void MakeUppercase(char *s)
{
    for (size_t i = 0; i < strnlen(s, MAX_PATH); i++)
    {
        s[i] = toupper(s[i]);
    }
}
