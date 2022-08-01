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

const DirEntry * GetRootDir(void)
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

const DirEntry * FindFile(const char *path)
{
    const size_t ClusterSize = GetClusterSize();

    char realPath[MAX_PATH];
    char nameBuf[MAX_SHORTNAME];
    char clustA[ClusterSize];
    char clustB[ClusterSize];
    char *clustBuf = clustA;

    strncpy(realPath, path, MAX_PATH);

    int depth = -1;
    char *tok = strtok(realPath, "/");

    const DirEntry *curDir = NULL;
    uint32_t cluster = 0;

    bool success = false;
    DirEntry *result = SafeAlloc(sizeof(DirEntry));

    while (tok != NULL)
    {
        depth++;

        bool isRoot = (curDir == NULL);
        int count = (isRoot)
            ?  GetBiosParams()->MaxRootDirEntryCount
            :  ClusterSize / sizeof(DirEntry);

        cluster = (isRoot) ? 0 : curDir->FirstCluster;
        success = false;

        do
        {
            if (!isRoot)
            {
                // load cluster
                clustBuf = (clustBuf == clustA) ? clustB : clustA;
                ReadCluster(clustBuf, cluster);
                cluster = GetNextCluster(cluster);
            }

            const DirEntry *e = (isRoot)
                ? s_pRootDir
                : (const DirEntry *) clustBuf;

            for (int i = 0; i < count; i++, e++)
            {
                switch ((unsigned char) e->Name[0])
                {
                    case 0x00:
                    case 0x05:
                    case 0xE5:
                        // free slot/deleted file
                        continue;
                }
                GetShortName(nameBuf, e);
                if (strcmp(nameBuf, tok) == 0)
                {
                    memcpy(result, e, sizeof(DirEntry));
                    success = true;
                    if (IsDirectory(e))
                    {
                        curDir = e;
                    }
                    break;
                }
            }

        } while (CLUSTER_IS_VALID(cluster));

        if (!success)
        {
            break;
        }

        tok = strtok(NULL, "/");
    }

    if (depth == -1)
    {
        SafeFree(result);
        return s_pRootDir;
    }

Cleanup:
    if (!success)
    {
        SafeFree(result);
        return NULL;
    }

    return result;
}

bool ReadCluster(char *dst, uint32_t index)
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

bool ReadFile(char *dst, const DirEntry *entry)
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

        RIF(ReadCluster(buf, cluster));
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
