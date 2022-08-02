#include "image.h"
#include "fatfs.h"
#include "fat12.h"

static BootSector s_BootSect;
static uint32_t s_NumClusters = 0;
static uint32_t *s_pClusterMap = NULL;
static DirEntry *s_pRootDir = NULL;
static FILE *s_FilePtr = NULL;
static char s_ImagePath[MAX_PATH];

static size_t CalculateClusterOffset(uint32_t cluster);
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
    fseek(s_FilePtr, sectorSize * (resSectors - 1), SEEK_CUR);

    // TODO: detect FAT type
    // For now, assume FAT12...

    // Load cluster map
    int fatSize = fatSectors * sectorSize;
    s_NumClusters = (fatSize / 3) * 2;   // 3 bytes = 24 bits = 2 FAT12 clusters
    int pairCount = s_NumClusters / 2;
    s_pClusterMap = SafeAlloc(s_NumClusters * sizeof(uint32_t));

    for (int i = 0; i < pairCount; i++)
    {
        uint8_t stride[3];
        SafeRead(s_FilePtr, stride, sizeof(stride));

        // |........|++++....|++++++++|
        uint32_t cluster0 = (stride[1] & 0xF) << 8 | stride[0];
        uint32_t cluster1 = (stride[2] << 4) | (stride[1] >> 4);

        s_pClusterMap[i * 2] = cluster0;
        s_pClusterMap[i * 2 + 1] = cluster1;
    }

    // Skip other copies of the FAT
    fseek(s_FilePtr, sectorSize * fatSectors * (numFats - 1), SEEK_CUR);
    // TODO: error detection/correction? option to select which FAT to read?

    // Load root directory
    int numDirEntries = GetBiosParams()->MaxRootDirEntryCount;
    int rootDirSize = numDirEntries * sizeof(DirEntry);
    s_pRootDir = SafeAlloc(rootDirSize);
    SafeRead(s_FilePtr, s_pRootDir, rootDirSize);

    // Save the disk image file path
    strncpy(s_ImagePath, path, MAX_PATH);

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

uint32_t GetNextCluster(uint32_t current)
{
    if (current >= s_NumClusters)
    {
        return CLUSTER_END;
    }

    return s_pClusterMap[current];
}

uint32_t GetFileSizeOnDisk(const DirEntry *file)
{
    const uint32_t ClusterSize = GetClusterSize();

    if (file == NULL)
    {
        return GetBiosParams()->MaxRootDirEntryCount * sizeof(DirEntry);
    }

    uint32_t size = 0;
    uint32_t cluster = file->FirstCluster;
    while (IsClusterValid(cluster))
    {
        size += ClusterSize;
        cluster = GetNextCluster(cluster);
    }

    return size;
}

bool FindFile(DirEntry **file, const char *path)
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
        } while (IsClusterValid(cluster));

        if (!success)
        {
            break;
        }

        tok = strtok(NULL, "/");
    }

    if (depth == -1)
    {
        SafeFree(result);
        *file = NULL;
        return true;
    }

Cleanup:
    if (!success)
    {
        SafeFree(result);
        *file = NULL;
        return false;
    }

    *file = result;
    return true;
}

bool ReadFile(char *dst, const DirEntry *file)
{
    const size_t ClusterSize = GetClusterSize();

    bool success = true;
    char buf[ClusterSize];
    uint32_t cluster = file->FirstCluster;
    size_t fileSize = file->FileSize;
    size_t totalBytesRead = 0;
    size_t readSize;

    bool isDir = IsDirectory(file);

    while (IsClusterValid(cluster))
    {
        readSize = (isDir)
            ? ClusterSize
            : min(ClusterSize, (fileSize - totalBytesRead));

        RIF(ReadCluster(buf, cluster));
        memcpy(dst, buf, readSize);

        dst += readSize;
        totalBytesRead += readSize;
        cluster = GetNextCluster(cluster);
    }

Cleanup:
    return success;
}

bool ReadCluster(char *dst, uint32_t index)
{
    const size_t ClusterSize = GetClusterSize();

    if (index >= s_NumClusters || !IsClusterValid(index))
    {
        LogWarning("attempt to read invalid data cluster 0x%03X\n", index);
        return false;
    }

    bool success = true;
    size_t seekAddr = CalculateClusterOffset(index);

    fseek(s_FilePtr, seekAddr, SEEK_SET);
    size_t bytesRead = SafeRead(s_FilePtr, dst, ClusterSize);
    assert(bytesRead == ClusterSize);

Cleanup:
    return success;
}

static inline size_t CalculateClusterOffset(uint32_t cluster)
{
    const BiosParamBlock *bpb = GetBiosParams();

    const size_t NumResSectors = bpb->ReservedSectorCount;
    const size_t NumFatSectors = bpb->TableCount * bpb->SectorsPerTable;
    const size_t SectorSize = bpb->SectorSize;
    const size_t ClusterSize = GetClusterSize();
    const size_t RootDirSize = GetFileSizeOnDisk(NULL);

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
