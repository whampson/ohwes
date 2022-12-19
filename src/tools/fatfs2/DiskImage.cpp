#include "DiskImage.hpp"

bool DiskImage::CreateNew(const char *path, const BiosParamBlock *bpb)
{
    FILE *fp = NULL;
    void *sectorBuf = NULL;
    void *clusterBuf = NULL;
    void *fat = NULL;
    int bytesWritten = 0;
    bool success = true;

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
    int sectorsUsed = resSectorCount + (fatSectorCount * fatCount) + rootSectorCount;
    int clustersFree = (sectorCount - Align(sectorsUsed, sectorsPerCluster)) / sectorsPerCluster;

    assert(sectorSize % 512 == 0);

    sectorsUsed += clustersFree * sectorsPerCluster;
    assert(sectorsUsed <= sectorCount);

    bool fat12 = clustersFree <= MAX_CLUSTER_12;
    int extraSectors = sectorCount - sectorsUsed;
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
    for (int i = 0; i < clustersFree; i++) {
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
            + (clustersFree * sectorsPerCluster)
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

    fp = SafeOpen(path, "rb", &size);
    SafeRIF(size >= 4096, "disk is too small\n");

    pos = 0;
    pos += SafeRead(fp, &bootSect, sizeof(BootSector));
    bpb = &bootSect.BiosParams;

    sectorSize = bpb->SectorSize;

    SafeRIF(IsPow2(sectorSize), "BPB is corrupt (sector size = %d)\n", sectorSize);
    SafeRIF(sectorSize >= MIN_SECTOR_SIZE, "BPB is corrupt (sector size = %d)\n", sectorSize);
    SafeRIF(sectorSize <= MAX_SECTOR_SIZE, "BPB is corrupt (sector size = %d)\n", sectorSize);
    SafeRIF(bpb->ReservedSectorCount > 0, "BPB is corrupt (reserved sector count = %d)\n", bpb->ReservedSectorCount);
    SafeRIF(bpb->RootDirCapacity > 0, "BPB is corrupt (root directory capacity = %d)\n", bpb->RootDirCapacity);
    SafeRIF(bpb->SectorsPerTable > 0, "BPB is corrupt (FAT sector count = %d)\n", bpb->SectorsPerTable);
    SafeRIF(bpb->TableCount > 0, "BPB is corrupt (FAT count = %d)\n", bpb->TableCount);

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

Cleanup:
    if (!success) {
        SafeFree(root);
        SafeFree(fat);
    }
    SafeFree(sectorBuf);
    SafeClose(fp);

    return (success) ? img : NULL;
}

DiskImage::DiskImage()
{
    m_Fat = NULL;
    m_Root = NULL;
}

DiskImage::~DiskImage()
{
    SafeFree(m_Fat);
    SafeFree(m_Root);
}

const BiosParamBlock * DiskImage::GetBPB() const
{
    return &m_Boot.BiosParams;
}

void DiskImage::PrintDiskInfo() const
{
    const BiosParamBlock *bpb = GetBPB();
    int sectorSize = bpb->SectorSize;
    int sectorCount = bpb->SectorCount;
    int sectorsPerCluster = bpb->SectorsPerCluster;
    int clusterSize = sectorSize * sectorsPerCluster;
    int fatSize = bpb->SectorsPerTable * sectorSize;
    int rootDirCapacity = bpb->RootDirCapacity;
    int rootSectorCount = Ceiling(rootDirCapacity * sizeof(DirEntry), sectorSize);
    int sectorsUsed = (bpb->ReservedSectorCount + (bpb->SectorsPerTable * bpb->TableCount) + rootSectorCount);
    int sectorsUsedAligned = Align(sectorsUsed, sectorsPerCluster);
    int clusters = (sectorCount - sectorsUsedAligned) / sectorsPerCluster;
    int bytesFree = clusters * clusterSize;

    int fatWidth = 0;
    if (clusters < MAX_CLUSTER_12) {
        fatWidth = 12;
    }
    else if (clusters < MAX_CLUSTER_16) {
        fatWidth = 16;
    }
    else {
        assert(!"unsupported FAT width!\n");
    }

    char labelBuf[MAX_LABEL];
    GetLabel(labelBuf, bpb->Label);

    LogInfo("%s statistics:\n", GetFileName(m_Path));
    LogInfo("%d %s, %d %s, %d %s per track\n",
        PluralForPrintf(bpb->SectorCount, "sector"),
        PluralForPrintf(bpb->HeadCount, "head"),
        PluralForPrintf(bpb->SectorsPerTrack, "sector"));
    LogInfo("%d byte sectors, %d %s per cluster\n",
        bpb->SectorSize,
        PluralForPrintf(bpb->SectorsPerCluster, "sector"));
    LogInfo("%d reserved %s\n",
        PluralForPrintf(bpb->ReservedSectorCount, "sector"));
    LogInfo("media type is 0x%02X, drive number is 0x%02X\n",
        bpb->MediaType, bpb->DriveNumber);
    LogInfo("%d %d-bit %s, %d %s per FAT, providing %d clusters\n",
        bpb->TableCount, fatWidth,
        Plural(bpb->TableCount, "FAT"),
        PluralForPrintf(fatSize / bpb->SectorSize, "sector"),
        clusters);
    LogInfo("root directory contains %d %s, occupying %d %s\n",
        PluralForPrintf(rootDirCapacity, "slot"),
        PluralForPrintf(rootSectorCount, "sector"));
    LogInfo("volume ID is %08X, volume label is '%s'\n",
        bpb->VolumeId, labelBuf);
    LogInfo("%d bytes free\n",
        bytesFree);
}
