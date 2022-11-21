#include "DiskImage.hpp"

bool DiskImage::Create(const char *path)
{
    FILE *fp = NULL;
    size_t bytesWritten = 0;
    bool success = true;

    BootSector bootSect;
    BiosParamBlock *bpb = &bootSect.BiosParams;
    char *fileAllocTable = NULL;
    char *rootDir = NULL;
    char *zeroSector = NULL;

    // FAT12
    // TODO: FAT16, FAT32?

    memset(&bootSect, 0, sizeof(BootSector));
    // InitBootSector(&bootSect);

    bpb->MediaType = MEDIA_TYPE_1440K;
    bpb->SectorSize = 512;
    bpb->SectorCount = 2880;
    bpb->ReservedSectorCount = 1;
    bpb->HiddenSectorCount = 0;
    bpb->LargeSectorCount = 0;
    bpb->SectorsPerCluster = 1;
    bpb->SectorsPerTable = 9;
    bpb->SectorsPerTrack = 18;
    bpb->TableCount = 2;
    bpb->MaxRootDirEntryCount = 224;
    bpb->HeadCount = 2;
    bpb->DriveNumber = 0;
    bpb->_Reserved = 0;
    bpb->ExtendedBootSignature = 0x29;
    bpb->VolumeId = time(NULL);
    strcpy(bpb->Label, DEFAULT_LABEL);
    strcpy(bpb->FileSystemType, DEFAULT_FS_TYPE);

    int sectorSize = bootSect.BiosParams.SectorSize;
    int sectorCount = bootSect.BiosParams.SectorCount;
    int diskSize = sectorCount * sectorSize;
    int rootSize = bootSect.BiosParams.MaxRootDirEntryCount * sizeof(DirEntry);
    int rootSectors = rootSize / sectorSize;
    int fatSectors = bootSect.BiosParams.SectorsPerTable;
    int fatCount = bootSect.BiosParams.TableCount;
    int fatSize = fatSectors * sectorSize;
    int resSectors = bootSect.BiosParams.ReservedSectorCount;
    int dataSectors = sectorCount - resSectors - (fatSectors * fatCount) - rootSectors;

    fileAllocTable = (char *) SafeAlloc(fatSize);
    rootDir = (char *) SafeAlloc(rootSize);
    zeroSector = (char *) SafeAlloc(sectorSize);

    memset(fileAllocTable, 0, fatSize);
    memset(rootDir, 0, rootSize);
    memset(zeroSector, 0, sectorSize);

    // File Allocation Table
    // Cluster 0: 0xFF0
    //     [7:0] - FAT ID / Media Type ID
    //    [11:9] - (padded with 1s)
    // Cluster 1: 0xFFF
    //    [11:0] - End-of-Chain marker
    fileAllocTable[0] = bootSect.BiosParams.MediaType;
    fileAllocTable[1] = 0xFF;
    fileAllocTable[2] = 0xFF;

    // The rest of the disk is zeroes. Easy!

    // Now, let's write the disk image.
    fp = SafeOpen(path, "wb");
    bytesWritten = 0;

    // The boot sector goes first, followed by any additional reserved sectors.
    // The boot sector is also a reserved sector.
    bytesWritten += SafeWrite(fp, &bootSect, sizeof(BootSector));
    for (int i = 1; i < resSectors; i++)
    {
        bytesWritten += SafeWrite(fp, zeroSector, sectorSize);
    }

    // Next, we write the FATs. Typically there are two for redundancy.
    for (int i = 0; i < fatCount; i++)
    {
        bytesWritten += SafeWrite(fp, fileAllocTable, fatSize);
    }

    // Now we do the root directory, which is all zeroes right now since this
    // is a brand-new disk.
    bytesWritten += SafeWrite(fp, rootDir, rootSize);

    // Last, zero-out the data area.
    for (int i = 0; i < dataSectors; i++)
    {
        bytesWritten += SafeWrite(fp, zeroSector, sectorSize);
    }

    // Sanity checks
    assert(bytesWritten % sectorSize == 0);
    assert(bytesWritten == (size_t) diskSize);

Cleanup:
    SafeClose(fp);
    SafeFree(zeroSector);
    SafeFree(rootDir);
    SafeFree(fileAllocTable);
    return success;
}

DiskImage::DiskImage(const char *path)
{
    // InitBootSector(&m_BootSect);

    (void) path;
    (void) m_BootSect;
    (void) m_FilePtr;
    (void) m_IsValid;
    (void) m_ImagePath;
}

DiskImage::~DiskImage()
{

}
