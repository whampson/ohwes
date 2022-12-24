#include "FatDisk.hpp"

bool FatDisk::CreateNew(const char *path, const BiosParamBlock *bpb)
{
    return CreateNew(path, bpb, 0);
}

bool FatDisk::CreateNew(const char *path, const BiosParamBlock *bpb, uint32_t sector)
{
    FILE *fp = NULL;
    void *sectorBuf = NULL;
    void *clusterBuf = NULL;
    char *fat = NULL;
    size_t bytesWritten = 0;
    bool success = true;

    if (bpb->SectorCount == 0 && bpb->SectorCountLarge == 0) {
        LogError("invalid BPB - sector count cannot be zero\n");
        return false;
    }
    if (bpb->SectorCount != 0 && bpb->SectorCountLarge != 0) {
        LogError("invalid BPB - only one 'SectorCount' field may be set\n");
        return false;
    }
    if (bpb->SectorsPerTable == 0) {
        LogError("invalid BPB - need at least one sector per FAT\n");
        // TODO: this field can be zero on FAT32, as long as the other one is set.
        return false;
    }
    if (bpb->SectorSize < MIN_SECTOR_SIZE) {
        LogError("invalid BPB - sector size must be at least 512\n");
        return false;
    }
    if (!IsPow2(bpb->SectorSize)) {
        LogError("invalid BPB - sector size must be a power of 2\n");
        return false;
    }
    if (!IsPow2(bpb->SectorsPerCluster)) {
        LogError("invalid BPB - sectors per cluster must be a power of 2\n");
        return false;
    }

    size_t baseAddr = sector * 512;   // Assume 512-byte sectors
    uint32_t sectorSize = bpb->SectorSize;
    uint32_t sectorCount = (bpb->SectorCount) ? bpb->SectorCount : bpb->SectorCountLarge;
    uint32_t sectorsPerCluster = bpb->SectorsPerCluster;
    uint32_t clusterSize = sectorSize * sectorsPerCluster;
    size_t diskSize = (bpb->SectorCount * sectorSize) + baseAddr;
    uint32_t resSectorCount = bpb->ReservedSectorCount;
    uint32_t fatSectorCount = bpb->SectorsPerTable;
    uint32_t fatSize = fatSectorCount * bpb->SectorSize;
    uint32_t fatCount = bpb->TableCount;
    uint32_t rootSize = bpb->RootDirCapacity * sizeof(DirEntry);
    uint32_t rootSectorCount = Ceiling(rootSize, sectorSize);
    uint32_t dataSectors = sectorCount - (resSectorCount + (fatSectorCount * fatCount) + rootSectorCount);
    uint32_t clusters = dataSectors / sectorsPerCluster;
    uint32_t extraSectors = dataSectors - (clusters * sectorsPerCluster);
    if (extraSectors) {
        LogWarning("disk has %d %s unreachable by FAT\n",
            PluralForPrintf(extraSectors, "sector"));
    }
    bool fat12 = clusters <= MAX_CLUSTER_12;
    bool hasCustomLabel = (bpb->Label[0] != ' ');

    fp = SafeOpen(path, "wb", NULL);
    sectorBuf = (char *) SafeAlloc(sectorSize);
    clusterBuf = (char *) SafeAlloc(clusterSize);
    fat = (char *) SafeAlloc(fatSize);

    if (baseAddr != 0) {
        LogVerbose("creating FAT file system at sector %d (address = 0x%zX)\n",
            sector, baseAddr);
        fseek(fp, baseAddr, SEEK_SET);
        bytesWritten = ftell(fp);
    }

    InitBootSector((BootSector *) sectorBuf, bpb, PROG_NAME);
    bytesWritten += SafeWrite(fp, sectorBuf, sectorSize);

    for (uint32_t i = 1; i < resSectorCount; i++) {
        bytesWritten += SafeWrite(fp, sectorBuf, sectorSize);
    }

    assert(bytesWritten == baseAddr + resSectorCount * sectorSize);

    if (fat12)
        InitFat12(fat, fatSize, bpb->MediaType);
    else
        InitFat16(fat, fatSize, bpb->MediaType);

    for (uint32_t i = 0; i < bpb->TableCount; i++) {
        bytesWritten += SafeWrite(fp, fat, fatSize);
    }

    assert(bytesWritten % sectorSize == 0);
    assert(bytesWritten ==
        baseAddr +
        (resSectorCount
            + (fatSectorCount * fatCount))
        * sectorSize);


    memset(sectorBuf, 0, sectorSize);
    for (uint32_t i = 0; i < rootSectorCount; i++) {
        if (hasCustomLabel && i == 0) {
            DirEntry volLabel;
            InitDirEntry(&volLabel);
            volLabel.Attributes = ATTR_LABEL;
            memcpy(sectorBuf, &volLabel, sizeof(DirEntry));
            bytesWritten += SafeWrite(fp, sectorBuf, sectorSize);
            memset(sectorBuf, 0, sectorSize);
        }
        else {
            bytesWritten += SafeWrite(fp, sectorBuf, sectorSize);
        }
    }

    assert(bytesWritten ==
        baseAddr +
        (resSectorCount
            + (fatSectorCount * fatCount)
            + rootSectorCount)
        * sectorSize);

    memset(clusterBuf, 0, clusterSize);
    for (uint32_t i = 0; i < clusters; i++) {
        bytesWritten += SafeWrite(fp, clusterBuf, clusterSize);
    }

    memset(sectorBuf, 0, sectorSize);
    for (uint32_t i = 0; i < extraSectors; i++) {
        bytesWritten += SafeWrite(fp, sectorBuf, sectorSize);
    }

    assert(bytesWritten % sectorSize == 0);
    assert(bytesWritten == diskSize);

    assert(bytesWritten ==
        baseAddr +
        (resSectorCount
            + (fatSectorCount * fatCount)
            + rootSectorCount
            + (clusters * sectorsPerCluster)
            + extraSectors)
        * sectorSize);

Cleanup:
    SafeClose(fp);
    SafeFree(clusterBuf);
    SafeFree(sectorBuf);
    SafeFree(fat);

    return success;
}

FatDisk * FatDisk::Open(const char *path)
{
    return Open(path, 0);
}

FatDisk * FatDisk::Open(const char *path, uint32_t sector)
{
    // TODO: handle partioned disks?

    BootSector bootSect;
    BiosParamBlock *bpb;

    FILE *fp = NULL;
    FatDisk *img = NULL;

    size_t pos;
    size_t size;
    size_t fsDataSize;
    uint32_t sectorSize;
    uint32_t rootSectorCount;

    void *sectorBuf = NULL;
    unsigned char *fat = NULL;
    unsigned char *root = NULL;

    bool success = true;
    size_t baseAddr = sector * 512;   // Assume 512-byte sectors

    fp = SafeOpen(path, "rb+", &size);
    SafeRIF(size + baseAddr >= 4096, "disk is too small\n");

    if (baseAddr != 0) {
        LogVerbose("looking for FAT file system at sector %d (address = 0x%zX)\n",
            sector, baseAddr);
        fseek(fp, baseAddr, SEEK_SET);
    }

    pos = 0;
    pos += SafeRead(fp, &bootSect, sizeof(BootSector));
    bpb = &bootSect.BiosParams;

    sectorSize = bpb->SectorSize;

    SafeRIF(IsPow2(sectorSize),
        "BPB is corrupt (sector size = %d)\n", sectorSize);
    SafeRIF(sectorSize >= MIN_SECTOR_SIZE,
        "BPB is corrupt (sector size = %d)\n", sectorSize);
    SafeRIF(sectorSize <= MAX_SECTOR_SIZE,
        "BPB is corrupt (sector size = %d)\n", sectorSize);
    SafeRIF(bpb->SectorCount != 0 || (bpb->SectorCount == 0 && bpb->SectorCountLarge != 0),
        "BPB is corrupt (sector count = %d\n", bpb->SectorCount);
    SafeRIF(bpb->ReservedSectorCount > 0,
        "BPB is corrupt (reserved sector count = %d)\n", bpb->ReservedSectorCount);
    SafeRIF(bpb->RootDirCapacity > 0,
        "BPB is corrupt (root directory capacity = %d)\n", bpb->RootDirCapacity);
    SafeRIF(bpb->SectorsPerTable > 0,
        "BPB is corrupt (FAT sector count = %d)\n", bpb->SectorsPerTable);
    SafeRIF(bpb->TableCount > 0,
        "BPB is corrupt (FAT count = %d)\n", bpb->TableCount);

    rootSectorCount = Ceiling(bpb->RootDirCapacity * sizeof(DirEntry), sectorSize);

    sectorBuf = SafeAlloc(sectorSize);
    fat = (unsigned char *) SafeAlloc(bpb->SectorsPerTable * sectorSize);
    root = (unsigned char *) SafeAlloc(rootSectorCount * sectorSize);

    if (sectorSize - pos > 0) {
        // skip the rest of the boot sector
        pos += SafeRead(fp, sectorBuf, sectorSize - pos);
    }

    fsDataSize = sectorSize *
        (bpb->ReservedSectorCount +
        (bpb->SectorsPerTable * bpb->TableCount) +
        rootSectorCount);

    SafeRIF(fsDataSize < size, "disk is too small\n");

    for (int i = 1; i < bpb->ReservedSectorCount; i++) {
        pos += SafeRead(fp, sectorBuf, sectorSize);
    }

    for (int i = 0; i < bpb->SectorsPerTable; i++) {
        pos += SafeRead(fp, fat + i * sectorSize, sectorSize);
    }

    // skip remaining FATs
    pos += (bpb->TableCount - 1) * bpb->SectorsPerTable * sectorSize;
    fseek(fp, pos, SEEK_SET);

    if (fat[0] != bpb->MediaType) {
        LogWarning("media type ID mismatch (FAT = 0x%02X, BPB = 0x%02X)\n",
            fat[0], bpb->MediaType);
    }

    for (uint32_t i = 0; i < rootSectorCount; i++) {
        pos += SafeRead(fp, root + i * sectorSize, sectorSize);
    }

    img = new FatDisk();
    img->m_BaseAddr = baseAddr;
    img->m_Boot = bootSect;
    img->m_Fat = (char *) fat;
    img->m_Path = path;
    img->m_File = fp;

Cleanup:
    SafeFree(sectorBuf);
    SafeFree(root);
    if (!success) {
        SafeFree(fat);
        SafeClose(fp);
    }

    return (success) ? img : NULL;
}

FatDisk::FatDisk()
{
    m_Fat = NULL;
    m_File = NULL;
}

FatDisk::~FatDisk()
{
    SafeFree(m_Fat);
    SafeClose(m_File);
}

bool FatDisk::IsFat12() const
{
    return GetClusterCount() <= MAX_CLUSTER_12;
}

bool FatDisk::IsFat16() const
{
    return GetClusterCount() >= MIN_CLUSTER_16
        && GetClusterCount() <= MAX_CLUSTER_16;
}

const BiosParamBlock * FatDisk::GetBPB() const
{
    return &m_Boot.BiosParams;
}

uint32_t FatDisk::GetDiskSize() const
{
    return GetSectorCount() * GetSectorSize();
}

uint32_t FatDisk::GetFileSize(const DirEntry *f) const
{
    size_t size = 0;

    if (IsRoot(f)) {
        size = GetRootCapacity() * sizeof(DirEntry);
    }
    else if (IsDirectory(f)) {
        uint32_t cluster = f->FirstCluster;
        do {
            size += GetClusterSize();
            cluster = GetCluster(cluster);
        } while (cluster != CLUSTER_EOC);
    }
    else {
        size = f->FileSize;
    }

    return size;
}

uint32_t FatDisk::GetSectorSize() const
{
    return GetBPB()->SectorSize;
}

uint32_t FatDisk::GetSectorCount() const
{
    const BiosParamBlock *bpb = GetBPB();
    uint32_t count = bpb->SectorCount;
    if (count == 0) {
        count = bpb->SectorCountLarge;
    }

    assert(count != 0);
    return count;
}

uint32_t FatDisk::GetClusterSize() const
{
    const BiosParamBlock *bpb = GetBPB();

    return bpb->SectorSize * bpb->SectorsPerCluster;
}

uint32_t FatDisk::GetClusterCount() const
{
    const BiosParamBlock *bpb = GetBPB();
    uint32_t fsDataSectors = bpb->ReservedSectorCount +
        (bpb->SectorsPerTable * bpb->TableCount) +
        Ceiling(bpb->RootDirCapacity * sizeof(DirEntry), bpb->SectorSize);

    return (GetSectorCount() - fsDataSectors) / bpb->SectorsPerCluster;
}

uint32_t FatDisk::GetFatCapacity() const
{
    const BiosParamBlock *bpb = GetBPB();
    uint32_t fatSize = bpb->SectorsPerTable * bpb->SectorSize;
    uint32_t fatCapacity = IsFat12()
        ? ((fatSize / 3) * 2) - FIRST_CLUSTER
        : (fatSize / 2) - FIRST_CLUSTER;

    return fatCapacity;
}

uint32_t FatDisk::GetRootCapacity() const
{
    return GetBPB()->RootDirCapacity;
}

DirEntry FatDisk::GetRootToken() const
{
    DirEntry e;
    memset(&e, 0, sizeof(DirEntry));

    e.Attributes = ATTR_DIRECTORY;
    e.FirstCluster = 0;

    return e;
}

uint32_t FatDisk::CountFreeClusters() const
{
    uint32_t free = 0;
    for (uint32_t i = 0; i < GetClusterCount(); i++) {
        uint32_t cluster = GetCluster(FIRST_CLUSTER + i);
        if (cluster == CLUSTER_FREE) {
            free++;
        }
    }

    return free;
}

uint32_t FatDisk::CountBadClusters() const
{
    uint32_t bad = 0;
    for (uint32_t i = 0; i < GetClusterCount(); i++) {
        uint32_t cluster = GetCluster(FIRST_CLUSTER + i);
        if (cluster == CLUSTER_BAD) {
            bad++;
        }
    }

    return bad;
}

uint32_t FatDisk::MarkClusterBad(uint32_t index)
{
    return SetCluster(index, CLUSTER_BAD);
}

uint32_t FatDisk::MarkClusterFree(uint32_t index)
{
    return SetCluster(index, CLUSTER_FREE);
}

uint32_t FatDisk::GetCluster(uint32_t index) const
{
    return IsFat12()
        ? GetCluster12(m_Fat, index)
        : GetCluster16(m_Fat, index);
}

uint32_t FatDisk::SetCluster(uint32_t index, uint32_t value)
{
    return IsFat12()
        ? SetCluster12(m_Fat, index, value)
        : SetCluster16(m_Fat, index, value);
}

bool FatDisk::ReadSector(char *dst, uint32_t index) const
{
    const uint32_t SectorSize = GetSectorSize();
    const uint32_t DiskSize = GetDiskSize();

    uint32_t seekAddr = m_BaseAddr + (index * SectorSize);
    if (seekAddr + SectorSize > DiskSize) {
        LogError("attempt to read out-of-bounds sector (index = %d)\n", index);
        return false;
    }

    bool success = true;
    fseek(m_File, seekAddr, SEEK_SET);
    SafeRead(m_File, dst, SectorSize);

Cleanup:
    return success;
}

bool FatDisk::ReadCluster(char *dst, uint32_t index) const
{
    const uint32_t ClusterSize = GetClusterSize();
    const uint32_t DiskSize = GetDiskSize();

    if ((index - FIRST_CLUSTER) > GetClusterCount()) {
        LogError("attempt to read out-of-bounds cluster (index = %0*X)\n",
            IsFat12() ? 3 : 4,
            index);
        return false;
    }

    const BiosParamBlock *bpb = GetBPB();
    uint32_t fsDataSectors = bpb->ReservedSectorCount +
        (bpb->SectorsPerTable * bpb->TableCount) +
        Ceiling(bpb->RootDirCapacity * sizeof(DirEntry), bpb->SectorSize);

    uint32_t seekAddr = m_BaseAddr +
        (fsDataSectors * bpb->SectorSize) +
        ((index - FIRST_CLUSTER) * ClusterSize);

    assert(seekAddr + ClusterSize <= DiskSize);

    bool success = true;
    fseek(m_File, seekAddr, SEEK_SET);
    SafeRead(m_File, dst, ClusterSize);

Cleanup:
    return success;
}

bool FatDisk::ReadFile(char *dst, const DirEntry *file) const
{
    bool success = true;

    if (IsRoot(file)) {
        const BiosParamBlock *bpb = GetBPB();
        int count = Ceiling(bpb->RootDirCapacity * sizeof(DirEntry), bpb->SectorSize);
        int base = bpb->ReservedSectorCount + (bpb->SectorsPerTable * bpb->TableCount);
        int i = 0;

        while (success && i < count) {
            success = ReadSector(dst + (i * GetSectorSize()), base + i);
            i++;
        }

        return success;
    }

    uint32_t cluster = file->FirstCluster;
    int i = 0;

    while (success && cluster != CLUSTER_EOC) {
        success = ReadCluster(dst + (i * GetClusterSize()), cluster);
        cluster = GetCluster(cluster);
        i++;
    }

    return success;
}

bool FatDisk::FindFile(DirEntry *dst, const char *path) const
{
    char pathCopy[MAX_PATH];
    strncpy(pathCopy, path, MAX_PATH);

    DirEntry root = GetRootToken();
    return WalkPath(dst, pathCopy, &root);
}

bool FatDisk::WalkPath(DirEntry *dst, char *path, const DirEntry *base) const
{
    char *nameToFind = strtok(path, "/\\");
    if (nameToFind == NULL) {
        memcpy(dst, base, sizeof(DirEntry));
        return true;
    }

    if (base == NULL || !HasAttribute(base, ATTR_DIRECTORY)) {
        LogError("attempt to walk path on non-directory\n");
        return false;
    }

    int count = GetFileSize(base) / sizeof(DirEntry);

    bool success = false;
    DirEntry *e = NULL;
    DirEntry *dirTable = NULL;

    dirTable = (DirEntry *) SafeAlloc(count * sizeof(DirEntry));
    SafeRIF(ReadFile((char *) dirTable, base), "failed to read directory\n");

    e = dirTable;
    for (int i = 0; i < count; i++, e++) {
        if (IsFree(e) || IsLongFileName(e)) {
            continue;
        }

        char shortName[MAX_SFN];
        GetShortFileName(shortName, e);

        if (strncasecmp(shortName, nameToFind, MAX_SFN) == 0) {
            success = WalkPath(dst, NULL, e); // NULL for recusive call
            break;
        }
    }

Cleanup:
    SafeFree(dirTable);
    return success;
}
