#include "DiskImage.hpp"

bool DiskImage::Create(
    const char *path,
    int sectorSize,
    int sectorCount,
    int headCount,
    int sectorsPerTrack,
    int sectorsPerCluster,
    int mediaType,
    int driveNumber,
    int fatCount,
    int fatWidth,
    int rootCapacity,
    int reservedCount,
    int hiddenCount,
    int volumeId,
    const char *label)
{
    FILE *fp = NULL;
    size_t bytesWritten = 0;
    bool success = true;

    BootSector bootSect;
    BiosParamBlock *bpb = NULL;
    char *fileAllocTable = NULL;
    char *rootDir = NULL;
    char *zeroSector = NULL;

    // FAT12
    // TODO: FAT16, FAT32

    //
    // Boot Sector
    //

    memset(&bootSect, 0, sizeof(BootSector));
    bootSect.BootSignature = BOOTSIG;

    static const char JumpCode[] =
    {
    /* entry:      jmp     boot_code    */  '\xEB', '\x3C',
    /*             nop                  */  '\x90'
    };
    static const char BootCode[] =
    {
    /* boot_code:  pushw   %cs          */  "\x0E"
    /*             popw    %ds          */  "\x1F"
    /*             leaw    message, %si */  "\x8D\x36\x1C\x00"
    /* print_loop: movb    $0x0e, %ah   */  "\xB4\x0E"
    /*             movw    $0x07, %bx   */  "\xBB\x07\x00"
    /*             lodsb                */  "\xAC"
    /*             andb    %al, %al     */  "\x20\xC0"
    /*             jz      key_press    */  "\x74\x04"
    /*             int     $0x10        */  "\xCD\x10"
    /*             jmp     print_loop   */  "\xEB\xF2"
    /* key_press:  xorb    %ah, %ah     */  "\x30\xE4"
    /*             int     $0x16        */  "\xCD\x16"
    /*             int     $0x19        */  "\xCD\x19"
    /* halt:       jmp     halt         */  "\xEB\xFE"
    /* message:    .ascii               */  "\r\nThis disk is not bootable!"
    /*             .asciz               */  "\r\nInsert a bootable disk and press any key to try again..."
    };

    static_assert(sizeof(JumpCode) <= sizeof(bootSect.JumpCode), "JumpCode is too large!");
    static_assert(sizeof(BootCode) <= sizeof(BootCode), "BootCode is too large!");

    memcpy(bootSect.BootCode, BootCode, sizeof(BootCode));
    memcpy(bootSect.JumpCode, JumpCode, sizeof(JumpCode));
    strcpy(bootSect.OemName, OEMNAME);

    //
    // BIOS Parameter Block
    //
    bpb = &bootSect.BiosParams;
    bpb->MediaType = mediaType;
    bpb->SectorSize = sectorSize;
    bpb->SectorCount = sectorCount;
    bpb->ReservedSectorCount = reservedCount;
    bpb->HiddenSectorCount = hiddenCount;
    bpb->SectorCountLarge = 0;  // TODO: set if sectorCount > 65535
    bpb->SectorsPerCluster = sectorsPerCluster;
    bpb->SectorsPerTable = 9;   // TODO: calculate
    bpb->SectorsPerTrack = sectorsPerTrack;
    bpb->TableCount = fatCount;
    bpb->RootDirCapacity = rootCapacity;
    bpb->HeadCount = headCount;
    bpb->DriveNumber = driveNumber;
    bpb->_Reserved = 0;
    bpb->Signature = 0x29;
    bpb->VolumeId = volumeId;

    strncpy(bpb->Label, label, LABEL_LENGTH);   // TODO: pad w/ spaces / truncate (create MakeLabel() fn)
    switch (fatWidth)
    {
        case 12: strcpy(bpb->FileSystemType, FSTYPEID_FAT12); break;
        case 16: strcpy(bpb->FileSystemType, FSTYPEID_FAT16); break;
        default: assert(!"Invalid FAT width!"); break;
    }

    // TODO: calculate sectors per FAT based on disk size
    const int sectorsPerTable = bpb->SectorsPerTable;

    // Some convenient variables
    int diskSize = sectorCount * sectorSize;
    int fatSize = sectorsPerTable * sectorSize;
    int rootSize = rootCapacity * sizeof(DirEntry);
    int rootSectors = rootSize / sectorSize;
    int dataSectors = sectorCount
        - bpb->ReservedSectorCount
        - (sectorsPerTable * bpb->TableCount)
        - rootSectors;

    // TODO: up-pad rootSize if needed

    assert(diskSize % 512 == 0);
    assert(diskSize % sectorSize == 0);
    assert(rootSize % sectorSize == 0);
    assert(fatSize % sectorSize == 0);

    //
    // FATs, Root Directory, and Data Area
    //

    // Allocate the necessary sections
    fileAllocTable = (char *) SafeAlloc(fatSize);
    rootDir = (char *) SafeAlloc(rootSize);
    zeroSector = (char *) SafeAlloc(sectorSize);

    memset(fileAllocTable, 0, fatSize);
    memset(rootDir, 0, rootSize);
    memset(zeroSector, 0, sectorSize);

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
    // |76543210|3210ba98|ba987654| :: bit index
    //
    fileAllocTable[0] = mediaType;
    fileAllocTable[1] = (char) (((CLUSTER_END & 0x00F) << 4) | 0x0F);
    fileAllocTable[2] = (char) (((CLUSTER_END & 0xFF0) >> 4));

    // The rest of the disk is zeroes. Easy!

    // Now, let's write the disk image.
    fp = SafeOpen(path, "wb");
    bytesWritten = 0;

    // The boot sector goes first, followed by any additional reserved sectors.
    // The boot sector is also a reserved sector, so we start this loop at 1.
    bytesWritten += SafeWrite(fp, &bootSect, sizeof(BootSector));
    for (int i = 1; i < reservedCount; i++)
    {
        bytesWritten += SafeWrite(fp, zeroSector, sectorSize);
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
        bytesWritten += SafeWrite(fp, zeroSector, sectorSize);
    }

    //
    // Done!
    //

    // Sanity checks
    assert(bytesWritten % sectorSize == 0);
    assert(bytesWritten == (size_t) diskSize);

    // User feedback
    // TODO: turn this into 'fatfs info'
    char labelBuf[MAX_LABEL];
    GetLabel(labelBuf, bpb->Label);

    LogInfo("%s statistics:\n",
        path);
    LogInfo("%d %s, %d %s, %d %s per track\n",
        PRINTF_PLURALIZE(bpb->SectorCount, "sector"),
        PRINTF_PLURALIZE(bpb->HeadCount, "head"),
        PRINTF_PLURALIZE(bpb->SectorsPerTrack, "sector"));
    LogInfo("%d byte sectors, %d %s per cluster\n",
        bpb->SectorSize,
        PRINTF_PLURALIZE(bpb->SectorsPerCluster, "sector"));
    LogInfo("%d reserved and %d hidden %s\n",
        bpb->ReservedSectorCount,
        PRINTF_PLURALIZE(bpb->HiddenSectorCount, "sector"));
    LogInfo("media type is 0x%02X\n",
        bpb->MediaType);
    LogInfo("drive number is %d\n",
        bpb->DriveNumber);
    LogInfo("signature byte is 0x%02X\n",
        bpb->Signature);
    LogInfo("%d %d-bit %s, occupying %d %s per FAT\n",
        bpb->TableCount, fatWidth,
        PLURALIZE(bpb->TableCount, "FAT"),
        PRINTF_PLURALIZE(fatSize / bpb->SectorSize, "sector"));
    LogInfo("root directory contains %d %s, occupying %d %s\n",
        PRINTF_PLURALIZE(rootCapacity, "slot"),
        PRINTF_PLURALIZE(rootSize / bpb->SectorSize, "sector"));
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
    (void) path;
    (void) m_BootSect;
    (void) m_FilePtr;
    (void) m_IsValid;
    (void) m_ImagePath;
}

DiskImage::~DiskImage()
{

}
