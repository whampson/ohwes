#include "DiskImage.hpp"

DiskImage * DiskImage::CreateNew(const char *path, const BiosParamBlock *bpb)
{
    FILE *fp = NULL;
    void *sectorBuf = NULL;
    void *clusterBuf = NULL;
    void *fat = NULL;
    int bytesWritten = 0;
    bool success = true;

    int sectorSize = bpb->SectorSize;
    int sectorCount = bpb->SectorCount;
    int sectorsPerCluster = bpb->SectorsPerCluster;
    int clusterSize = sectorSize * sectorsPerCluster;
    int diskSize = bpb->SectorCount * sectorSize;
    int resSectorCount = bpb->ReservedSectorCount;
    int fatSectorCount = bpb->SectorsPerTable;
    int fatSize = fatSectorCount * bpb->SectorSize;
    int fatCount = bpb->TableCount;
    int rootSize = bpb->RootDirCapacity * sizeof(DirEntry);
    int rootSectorCount = rootSize / sectorSize;
    int sectorsUsed = resSectorCount + (fatSectorCount * fatCount) + rootSectorCount;

    int clustersFree = (sectorCount - Align(sectorsUsed, sectorsPerCluster)) / sectorsPerCluster;

    sectorsUsed += clustersFree * sectorsPerCluster;
    assert(sectorsUsed <= sectorCount);

    int extraSectors = sectorCount - sectorsUsed;

    bool fat12 = clustersFree <= MAX_CLUSTER_12;

    fp = SafeOpen(path, "wb");
    sectorBuf = (char *) SafeAlloc(sectorSize);
    clusterBuf = (char *) SafeAlloc(clusterSize);
    fat = (char *) SafeAlloc(fatSize);

    LogVerbose("Writing reserved area...\n");
    InitBootSector((BootSector *) sectorBuf, bpb);
    bytesWritten += SafeWrite(fp, sectorBuf, sectorSize);

    for (int i = 1; i < resSectorCount; i++) {
        bytesWritten += SafeWrite(fp, sectorBuf, sectorSize);
    }

    assert(bytesWritten == resSectorCount * sectorSize);

    LogVerbose("Writing FAT area...\n");
    InitFileAllocTable(fat, fatSize, bpb->MediaType, fat12);
    for (int i = 0; i < bpb->TableCount; i++) {
        bytesWritten += SafeWrite(fp, fat, fatSize);
    }

    assert(bytesWritten % sectorSize == 0);
    assert(bytesWritten ==
        (resSectorCount
            + (fatSectorCount * fatCount))
        * sectorSize);

    LogVerbose("Writing root directory...\n");
    for (int i = 0; i < rootSectorCount; i++) {
        bytesWritten += SafeWrite(fp, sectorBuf, sectorSize);
    }

    assert(bytesWritten ==
        (resSectorCount
            + (fatSectorCount * fatCount)
            + rootSectorCount)
        * sectorSize);

    LogVerbose("Writing data area...\n");
    memset(clusterBuf, 0, clusterSize);
    for (int i = 0; i < clustersFree; i++) {
        LogVerbose("%d: ", i);
        bytesWritten += SafeWrite(fp, clusterBuf, clusterSize);
    }

    if (extraSectors) LogVerbose("Writing extra sectirs...\n");
    memset(sectorBuf, 0, sectorSize);
    for (int i = 0; i < extraSectors; i++) {
        LogVerbose("%d: ", i);
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
    SafeFree(clusterBuf);
    SafeFree(sectorBuf);
    SafeFree(fat);

    return new DiskImage(); // TODO
}

DiskImage * DiskImage::Open(const char *path)
{
    (void) path;

    return NULL;
}
