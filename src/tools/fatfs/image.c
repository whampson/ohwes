#include "image.h"
#include "fatfs.h"
#include "fat12.h"

static BootSector s_BootSect;
static uint32_t s_NumClusters = 0;
static uint32_t *s_pClusterMap = NULL;
static FILE *s_FilePtr = NULL;
static char s_ImagePath[MAX_PATH];

static DirEntry s_RootDir =
{
    .Attributes = ATTR_DIRECTORY,
    .FileSize = 0,
    .FirstCluster = 0,
};

static size_t GetClusterOffset(uint32_t cluster);
static void MakeUppercase(char *s);
static bool TraversePath(DirEntry *file, char *path, const DirEntry *dir);

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
    // TODO: FAT error detection/correction? option to select which FAT to read?

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
    return &s_RootDir;
}

const uint32_t * GetClusterMap(void)
{
    return s_pClusterMap;
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

uint32_t GetFileSize(const DirEntry *file)
{
    if (IsDirectory(file))
    {
        return GetFileSizeOnDisk(file);
    }

    return file->FileSize;
}

uint32_t GetFileSizeOnDisk(const DirEntry *file)
{
    const uint32_t ClusterSize = GetClusterSize();

    if (IsRoot(file))
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

bool FindFileInDir(DirEntry *file, const char *path, const DirEntry *dir)
{
    char pathCopy[MAX_PATH];
    strncpy(pathCopy, path, MAX_PATH);

    return TraversePath(file, pathCopy, dir);
}

bool FindFile(DirEntry *file, const char *path)
{
    return FindFileInDir(file, path, GetRootDir());
}

bool ReadFile(char *dst, const DirEntry *file)
{
    const size_t ClusterSize = GetClusterSize();

    bool success = true;
    char buf[ClusterSize];

    if (IsRoot(file))
    {
        size_t rootSize = GetFileSize(GetRootDir());
        size_t addr = GetClusterOffset(CLUSTER_FIRST) - rootSize;
        fseek(s_FilePtr, addr, SEEK_SET);

        SafeRead(s_FilePtr, dst, rootSize);
        return true;
    }

    uint32_t cluster = file->FirstCluster;
    size_t fileSize = file->FileSize;
    size_t totalBytesRead = 0;
    size_t readSize;

    while (IsClusterValid(cluster))
    {
        readSize = IsDirectory(file)
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

static inline size_t GetClusterOffset(uint32_t cluster)
{
    const BiosParamBlock *bpb = GetBiosParams();

    const size_t NumResSectors = bpb->ReservedSectorCount;
    const size_t NumFatSectors = bpb->TableCount * bpb->SectorsPerTable;
    const size_t SectorSize = bpb->SectorSize;
    const size_t ClusterSize = GetClusterSize();
    const size_t RootDirSize = GetFileSizeOnDisk(GetRootDir());

    return ((NumResSectors + NumFatSectors) * SectorSize)
        + RootDirSize
        + ((cluster - 2) * ClusterSize);
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
    size_t seekAddr = GetClusterOffset(index);

    fseek(s_FilePtr, seekAddr, SEEK_SET);
    size_t bytesRead = SafeRead(s_FilePtr, dst, ClusterSize);
    assert(bytesRead == ClusterSize);

Cleanup:
    return success;
}

bool ReadLongName(wchar_t dst[MAX_PATH], char *cksum, const DirEntry **entry)
{
    bool hasLfn = false;
    wchar_t buildBuf[MAX_PATH];
    memset(buildBuf, 0xFF, sizeof(buildBuf));

    const DirEntry *e = *entry;
    while (IsLongFileName(e) && !IsDeleted(e))
    {
        hasLfn = true;
        if (cksum) *cksum = e->LFN.Checksum;

        int b = (e->LFN.Sequence - 1) * (LFN_CAPACITY * sizeof(wchar_t));
        for (int k = 0; k < LFN_CAPACITY; k++)
        {
            wchar_t c = 0;
            if (k < 5)              c = e->LFN.NameChunk1[k];
            if (k >= 5 && k < 11)   c = e->LFN.NameChunk2[k - 5];
            if (k >= 11)            c = e->LFN.NameChunk3[k - 11];

            buildBuf[b + k] = c;
        }
        e++;
        continue;
    }

    if (hasLfn)
    {
        int n = 0;
        for (int k = 0; k < MAX_PATH; k++)
        {
            wchar_t c = buildBuf[k];
            if (c == 0xFFFF) continue;
            if ((dst[n++] = c) == 0) break;
        }
        --e;
    }

    *entry = e;
    return hasLfn;
}

static bool TraversePath(DirEntry *file, char *path, const DirEntry *dir)
{
    bool success = false;
    char shortname[MAX_SHORTNAME];

    char *tok = strtok(path, "/");
    if (tok == NULL)
    {
        // we got it!
        success = true;
        memcpy(file, dir, sizeof(DirEntry));
        goto Cleanup;
    }

    wchar_t wtok[MAX_PATH];
    mbstowcs(wtok, tok, MAX_PATH);

    size_t size = GetFileSize(dir);
    DirEntry *dirTable = SafeAlloc(size);
    RIF(ReadFile((char *) dirTable, dir));

    const DirEntry *e = dirTable;
    int count = size / sizeof(DirEntry);

    // TODO: handle . and .. in root? (self-referential)

    char lfnCksum = 0;
    wchar_t lfn[MAX_PATH];
    bool hasLfn = false;

    for (int i = 0; i < count; i++, e++)
    {
        if (IsLongFileName(e) && !IsDeleted(e))
        {
            hasLfn = ReadLongName(lfn, &lfnCksum, &e);
            continue;
        }
        else if (!IsFile(e))
        {
            hasLfn = false;
            continue;
        }

        GetShortName(shortname, e);

        bool match = false;
        if (hasLfn && wcscmp(lfn, wtok) == 0)
        {
            if (GetShortNameChecksum(e) == lfnCksum)
            {
                match = true;
            }
        }
        else if (strcmp(shortname, tok) == 0)
        {
            match = true;
        }

        if (match)
        {
            success = TraversePath(file, NULL, e);  // NULL for recursive call
            break;
        }
    }

Cleanup:
    SafeFree(dirTable);
    return success;
}

static void MakeUppercase(char *s)
{
    for (size_t i = 0; i < strnlen(s, MAX_PATH); i++)
    {
        s[i] = toupper(s[i]);
    }
}
