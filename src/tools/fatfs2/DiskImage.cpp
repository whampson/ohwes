#include "DiskImage.hpp"

bool DiskImage::CreateNew(const char *path, const BiosParamBlock *bpb)
{
    FILE *fp = NULL;
    void *sectorBuf = NULL;
    void *clusterBuf = NULL;
    void *fat = NULL;
    int bytesWritten = 0;
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

    int sectorSize = bpb->SectorSize;
    int sectorCount = (bpb->SectorCount) ? bpb->SectorCount : bpb->SectorCountLarge;
    int sectorsPerCluster = bpb->SectorsPerCluster;
    int clusterSize = sectorSize * sectorsPerCluster;
    int diskSize = bpb->SectorCount * sectorSize;
    int resSectorCount = bpb->ReservedSectorCount;
    int fatSectorCount = bpb->SectorsPerTable;
    int fatSize = fatSectorCount * bpb->SectorSize;
    int fatCount = bpb->TableCount;
    int rootSize = bpb->RootDirCapacity * sizeof(DirEntry);
    int rootSectorCount = Ceiling(rootSize, sectorSize);
    int dataSectors = sectorCount - (resSectorCount + (fatSectorCount * fatCount) + rootSectorCount);
    int clusters = dataSectors / sectorsPerCluster;
    int extraSectors = dataSectors - (clusters * sectorsPerCluster);
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

    InitBootSector((BootSector *) sectorBuf, bpb);
    bytesWritten += SafeWrite(fp, sectorBuf, sectorSize);

    for (int i = 1; i < resSectorCount; i++) {
        bytesWritten += SafeWrite(fp, sectorBuf, sectorSize);
    }

    assert(bytesWritten == resSectorCount * sectorSize);

    InitFileAllocTable(fat, fatSize, bpb->MediaType, fat12);
    for (int i = 0; i < bpb->TableCount; i++) {
        bytesWritten += SafeWrite(fp, fat, fatSize);
    }

    assert(bytesWritten % sectorSize == 0);
    assert(bytesWritten ==
        (resSectorCount
            + (fatSectorCount * fatCount))
        * sectorSize);


    memset(sectorBuf, 0, sectorSize);
    for (int i = 0; i < rootSectorCount; i++) {
        if (hasCustomLabel && i == 0) {
            DirEntry volLabel;
            MakeVolumeLabel(&volLabel, bpb->Label);
            memcpy(sectorBuf, &volLabel, sizeof(DirEntry));
            bytesWritten += SafeWrite(fp, sectorBuf, sectorSize);
            memset(sectorBuf, 0, sectorSize);
        }
        else {
            bytesWritten += SafeWrite(fp, sectorBuf, sectorSize);
        }
    }

    assert(bytesWritten ==
        (resSectorCount
            + (fatSectorCount * fatCount)
            + rootSectorCount)
        * sectorSize);

    memset(clusterBuf, 0, clusterSize);
    for (int i = 0; i < clusters; i++) {
        bytesWritten += SafeWrite(fp, clusterBuf, clusterSize);
    }

    memset(sectorBuf, 0, sectorSize);
    for (int i = 0; i < extraSectors; i++) {
        bytesWritten += SafeWrite(fp, sectorBuf, sectorSize);
    }

    assert(bytesWritten % sectorSize == 0);
    assert(bytesWritten == diskSize);

    assert(bytesWritten ==
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

DiskImage * DiskImage::Open(const char *path)
{
    // TODO: handle partioned disks?

    BootSector bootSect;
    BiosParamBlock *bpb;

    FILE *fp = NULL;
    DiskImage *img = NULL;

    size_t pos;
    size_t size;
    size_t fsSize;
    int sectorSize;
    int rootSectorCount;

    void *sectorBuf = NULL;
    unsigned char *fat = NULL;
    unsigned char *root = NULL;

    bool success = true;

    fp = SafeOpen(path, "rb+", &size);
    SafeRIF(size >= 4096, "disk is too small\n");

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

    fsSize = sectorSize *
        (bpb->ReservedSectorCount +
        (bpb->SectorsPerTable * bpb->TableCount) +
        rootSectorCount);

    SafeRIF(fsSize < size, "disk is too small\n");

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
        LogWarning("media type ID mismatch (FAT = 0x%02X, BPB = 0x%02X)\n", fat[0], bpb->MediaType);
    }

    for (int i = 0; i < rootSectorCount; i++) {
        pos += SafeRead(fp, root + i * sectorSize, sectorSize);
    }

    img = new DiskImage();
    img->m_Boot = bootSect;
    img->m_Fat = fat;
    img->m_Root = (DirEntry *) root;
    img->m_Path = path;
    img->m_File = fp;

Cleanup:
    SafeFree(sectorBuf);
    if (!success) {
        SafeFree(root);
        SafeFree(fat);
        SafeClose(fp);
    }

    return (success) ? img : NULL;
}

DiskImage::DiskImage()
{
    m_Fat = NULL;
    m_Root = NULL;
    m_File = NULL;
}

DiskImage::~DiskImage()
{
    SafeFree(m_Fat);
    SafeFree(m_Root);
    SafeClose(m_File);
}

const BiosParamBlock * DiskImage::GetBPB() const
{
    return &m_Boot.BiosParams;
}

int DiskImage::GetSectorSize() const
{
    const BiosParamBlock *bpb = GetBPB();

    return bpb->SectorSize;
}

int DiskImage::GetSectorCount() const
{
    const BiosParamBlock *bpb = GetBPB();
    int count = bpb->SectorCount;
    if (count == 0) {
        count = bpb->SectorCountLarge;  // TODO: unsigned?
    }

    assert(count != 0);
    return count;
}

int DiskImage::GetClusterSize() const
{
    const BiosParamBlock *bpb = GetBPB();

    return bpb->SectorSize * bpb->SectorsPerCluster;
}

int DiskImage::GetClusterCount() const
{
    const BiosParamBlock *bpb = GetBPB();
    int dataSectors =
        GetSectorCount() -
        (bpb->ReservedSectorCount +
        (bpb->SectorsPerTable * bpb->TableCount) +
        Ceiling(bpb->RootDirCapacity * sizeof(DirEntry), bpb->SectorSize));

    return dataSectors / bpb->SectorsPerCluster;
}

bool DiskImage::IsFat12() const
{
    return GetClusterCount() <= MAX_CLUSTER_12;
}

