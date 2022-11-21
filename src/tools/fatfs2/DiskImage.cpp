#include "DiskImage.hpp"

bool DiskImage::Create(
    const char *path,
    const char *label,
    int fatWidth,
    int mediaType)
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
    // TODO: FAT16, FAT32

    memset(&bootSect, 0, sizeof(BootSector));

    // BIOS Parameter Block
    bpb->MediaType = mediaType;
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
    strncpy(bpb->Label, label, LABEL_LENGTH);
    strcpy(bpb->FileSystemType, DEFAULT_FS_TYPE);

    // Some convenient variables
    int diskSize = bpb->SectorCount * bpb->SectorSize;
    int clusterSize = bpb->SectorsPerCluster * bpb->SectorSize;
    int rootSize = bpb->MaxRootDirEntryCount * sizeof(DirEntry);
    int rootSectors = rootSize / bpb->SectorSize;
    int fatSize = bpb->SectorsPerTable * bpb->SectorSize;
    int dataSectors = bpb->SectorCount
        - bpb->ReservedSectorCount
        - (bpb->SectorsPerTable * bpb->TableCount)
        - rootSectors;

    // Allocate the necessary sections
    fileAllocTable = (char *) SafeAlloc(fatSize);
    rootDir = (char *) SafeAlloc(rootSize);
    zeroSector = (char *) SafeAlloc(bpb->SectorSize);

    memset(fileAllocTable, 0, fatSize);
    memset(rootDir, 0, rootSize);
    memset(zeroSector, 0, bpb->SectorSize);

    //
    // File Allocation Table (FAT12)
    //
    // Cluster 0:
    //     [7:0] - Media Type ID
    //    [11:9] - (padded with 1s)
    // Cluster 1:
    //    [11:0] - End-of-Chain marker
    //
    //     0        1        2      :: byte index
    // |........|++++....|++++++++| :: . = clust0, + = clust1
    // |76543210|3210ba98|ba987654| :: bit index of clustID
    //
    fileAllocTable[0] = mediaType;
    fileAllocTable[1] = (char) (((CLUSTER_END & 0x00F) << 4) | 0x0F);
    fileAllocTable[2] = (char) (((CLUSTER_END & 0xFF0) >> 4));

    //
    // The rest of the disk is zeroes. Easy!
    //

    // Now, let's write the disk image.
    fp = SafeOpen(path, "wb");
    bytesWritten = 0;

    // The boot sector goes first, followed by any additional reserved sectors.
    // The boot sector is also a reserved sector, so we start this loop at 1.
    bytesWritten += SafeWrite(fp, &bootSect, sizeof(BootSector));
    for (int i = 1; i < bpb->ReservedSectorCount; i++)
    {
        bytesWritten += SafeWrite(fp, zeroSector, bpb->SectorSize);
    }

    // Next, we write the FATs. Typically there are two for redundancy.
    for (int i = 0; i < bpb->TableCount; i++)
    {
        bytesWritten += SafeWrite(fp, fileAllocTable, fatSize);
    }

    // Now we do the root directory, which is all zeroes right now since this
    // is a brand-new disk.
    bytesWritten += SafeWrite(fp, rootDir, rootSize);

    // Lastly, zero-out the data area.
    for (int i = 0; i < dataSectors; i++)
    {
        bytesWritten += SafeWrite(fp, zeroSector, bpb->SectorSize);
    }

    //
    // Done!
    //

    // Sanity checks
    assert(bytesWritten % bpb->SectorSize == 0);
    assert(bytesWritten == (size_t) diskSize);

    // User feedback
    char labelBuf[MAX_LABEL];
    GetLabel(labelBuf, bpb->Label);

    LogInfo("%s statistics:\n",
        path);
    LogInfo("%d sectors, %d heads, %d sectors per track\n",
        bpb->SectorCount, bpb->HeadCount, bpb->SectorsPerTrack);
    LogInfo("%d byte sectors, %d byte clusters\n",
        bpb->SectorSize, clusterSize);
    LogInfo("%d reserved, %d hidden, and %d large sectors\n",
        bpb->ReservedSectorCount, bpb->HiddenSectorCount, bpb->LargeSectorCount);
    LogInfo("media type ID is 0x%02X\n",
        bpb->MediaType);
    LogInfo("drive number is %d\n",
        bpb->DriveNumber);
    LogInfo("extended boot signature is 0x%02X\n",
        bpb->ExtendedBootSignature);
    LogInfo("%d %d-bit FATs, %d sectors each\n",
        bpb->TableCount, fatWidth, fatSize / bpb->SectorSize);
    LogInfo("root directory capacity is %d, occupying %d sectors\n",
        bpb->MaxRootDirEntryCount, rootSize / bpb->SectorSize);
    LogInfo("volume ID is %08X, volume label is '%s'\n",
        bpb->VolumeId, labelBuf);
    LogInfo("%d bytes free\n",
        dataSectors * bpb->SectorSize);

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
