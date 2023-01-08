#include "Command.hpp"
#include "FatDisk.hpp"

int Create(const Command *cmd, const CommandArgs *args)
{
    (void) cmd;
    const char *path = NULL;

    // Defaults for 3.5" double-sided 1440k floppy disk
    // TODO: select geometry & drive number based on media type
    //       drive number for media type 0xF8 should be 0x80 (hard disk)
    // TODO: select media type based on geometry

    int sectorSize = 512;
    int sectorCount = 2880;     // TODO: unsigned?
    int headCount = 2;
    int sectorsPerTrack = 18;
    int sectorsPerCluster = 1;
    int mediaType = MEDIATYPE_1440K;
    int driveNumber = 0;
    int fatCount = 2;
    int fatWidth = 0;   // autoselect
    int rootDirCapacity = 224;
    int reservedSectorCount = 1;
    int volumeId = time(NULL);
    const char *label = "";

    int force = 0;
    int noAlign = 0;

    static struct option LongOptions[] = {
        GLOBAL_LONGOPTS,
        { "force", no_argument, &force, 1 },
        { "no-align", no_argument, &noAlign, 1 },
        { 0, 0, 0, 0 }
    };

    optind = 0;     // getopt: reset option index
    opterr = 0;     // getopt: prevent default error messages
    optidx = 0;     // reset option index

    // Parse option arguments
    while (true) {
        int c = getopt_long(
            args->Argc, args->Argv,
            "+:d:f:F:g:i:l:m:r:R:s:S:",
            LongOptions, &optidx);

        if (c == -1) break;
        ProcessGlobalOption(args->Argv, LongOptions, c);

        switch (c) {
            case 'd':
                driveNumber = (char) strtol(optarg, NULL, 0);
                break;
            case 'f':
                fatCount = (int) strtol(optarg, NULL, 0);
                break;
            case 'F':
                fatWidth = (int) strtol(optarg, NULL, 0);
                break;
            case 'g':
                optarg = strtok(optarg, "/");
                if (!optarg) {
                    LogError("invalid geometry format\n");
                    return STATUS_INVALIDARG;
                }
                headCount = strtol(optarg, NULL, 0);
                optarg = strtok(NULL, "/");
                if (!optarg) {
                    LogError("invalid geometry format\n");
                    return STATUS_INVALIDARG;
                }
                sectorsPerTrack = (int) strtol(optarg, NULL, 0);
                break;
            case 'i':
                volumeId = (int) strtol(optarg, NULL, 16);
                break;
            case 'l':
                label = optarg;
                break;
            case 'm':
                mediaType = (char) strtol(optarg, NULL, 0);
                break;
            case 'r':
                rootDirCapacity = (int) strtol(optarg, NULL, 0);
                break;
            case 'R':
                reservedSectorCount = (int) strtol(optarg, NULL, 0);
                break;
            case 's':
                sectorsPerCluster = (int) strtol(optarg, NULL, 0);
                break;
            case 'S':
                sectorSize = (int) strtol(optarg, NULL, 0);
                break;
        }
    }

    // Parse positional arguments
    int pos = 0;
    while (optind < args->Argc) {
        optarg = args->Argv[optind++];
        switch (pos++) {
            case 0:
                path = optarg;
                break;
            case 1:
                sectorCount = (int) strtol(optarg, NULL, 0);
                break;
            default:
                LogError_BadArg(optarg);
                return STATUS_INVALIDARG;
                break;
        }
    }

    CheckParam(path != NULL, "missing disk image file name\n");
    CheckParam(IsPow2(sectorSize), "sector size must be a power of 2\n");
    CheckParam(sectorSize >= MIN_SECTOR_SIZE, "sector size must be at least %d bytes\n", MIN_SECTOR_SIZE);
    CheckParam(sectorSize <= MAX_SECTOR_SIZE, "sector size is too large\n");
    CheckParam(sectorCount > 0, "invalid sector count\n");
    CheckParam(headCount > 0, "invalid head count\n");
    CheckParam(sectorsPerTrack > 0, "invalid sectors per track\n");
    CheckParam(IsPow2(sectorsPerCluster), "sector size must be a power of 2\n");
    CheckParam(sectorsPerCluster <= MAX_SEC_PER_CLUST, "too many sectors per cluster\n");
    CheckParam(fatCount >= 1, "at least one file allocation table is required\n");
    CheckParam(fatWidth == 0 || fatWidth == 12 || fatWidth == 16, "invalid FAT width, must be 12 or 16\n");
    CheckParam(rootDirCapacity > 0, "invalid root directory capacity\n");
    CheckParam(reservedSectorCount >= 1, "at least 1 reserved sector is required\n");

    // Test whether the file exists,
    // fail if it does and --force not specified
    if (FileExists(path) && !force) {
        LogError("'%s' exists\n", path);
        return STATUS_ERROR;
    }

    if (sectorCount <= 4096) {
        noAlign = 1;
        LogVerbose("disabling alignment for small disk\n");
    }

    int rootSectorCount = CeilDiv(rootDirCapacity * sizeof(DirEntry), sectorSize);
    int sectorsUsed = rootSectorCount + reservedSectorCount;

    if (!noAlign) {
        sectorCount = Align(sectorCount, sectorsPerCluster);
        rootDirCapacity = (rootSectorCount * sectorSize) / sizeof(DirEntry);
    }

    int fatSize = 0;
    int clusters = 0;
    bool fatSizeKnown = false;

    // Figure out how big the FAT needs to be to address all clusters on disk.
    do {
        sectorsUsed += fatCount;
        fatSize += sectorSize;

        int sectorsUsedAligned = Align(sectorsUsed, sectorsPerCluster);
        clusters = (sectorCount - sectorsUsedAligned) / sectorsPerCluster;

        int fatCapacity12 = ((fatSize / 3) * 2) - CLUSTER_FIRST;
        int fatCapacity16 = (fatSize / 2) - CLUSTER_FIRST;

        bool maybeFat12 = (fatWidth == 0 || fatWidth == 12);
        bool maybeFat16 = (fatWidth == 0 || fatWidth == 16);

        if (clusters > MAX_CLUSTERS_12 && fatCapacity12 > MAX_CLUSTERS_12) {
            // TODO: we could squeeze extra clusters out of the 12-bit FAT
            // by properly handling the sector boundaries
            if (fatWidth == 12) {
                LogError("too many clusters for FAT12\n");
                return STATUS_ERROR;
            }
            maybeFat12 = false;
        }

        if (clusters > MAX_CLUSTERS_16 && fatCapacity16 > MAX_CLUSTERS_16) {
            if (fatWidth == 16) {
                LogError("too many clusters for FAT16\n");
            }
            else {
                LogError("disk is too large\n");
            }
            return STATUS_ERROR;
        }

        if (maybeFat12 && clusters <= fatCapacity12) {
            if (fatWidth == 0) {
                LogVerbose("selecting FAT12 because %d < %d clusters\n", clusters, MIN_CLUSTERS_16);
            }
            if (clusters < MIN_CLUSTERS_12)  {
                LogError("not enough clusters for FAT12\n");
                return STATUS_ERROR;
            }
            fatWidth = 12;
            fatSizeKnown = true;
        }
        else if (maybeFat16 && clusters <= fatCapacity16) {
            if (fatWidth == 0 && clusters >= MIN_CLUSTERS_16) {
                LogVerbose("selecting FAT16 because %d >= %d clusters\n", clusters, MIN_CLUSTERS_16);
            }
            if (fatWidth == 16 && clusters < MIN_CLUSTERS_16) {
                LogError("not enough clusters for FAT16\n");
                return STATUS_ERROR;
            }
            fatWidth = 16;
            fatSizeKnown = true;
        }
    } while (!fatSizeKnown);

    // Create the BPB
    struct BiosParamBlock bpb;
    InitBiosParamBlock(&bpb);
    bpb.MediaType = mediaType;
    bpb.HeadCount = headCount;
    bpb.DriveNumber = driveNumber;
    bpb.SectorCount = sectorCount;
    bpb.SectorSize = sectorSize;
    bpb.TableCount = fatCount;
    bpb.SectorsPerTable = fatSize / sectorSize;
    bpb.SectorsPerTrack = sectorsPerTrack;
    bpb.SectorsPerCluster = sectorsPerCluster;
    bpb.RootDirCapacity = rootDirCapacity;
    bpb.ReservedSectorCount = reservedSectorCount;
    bpb.HiddenSectorCount = 0;  // not supported unless disk is partitioned
    bpb.Signature = BPBSIG_DOS41;
    bpb.VolumeId = volumeId;

    if (sectorCount > 65535) {
        bpb.SectorCount = 0;
        bpb.SectorCountLarge = sectorCount;
    }

    WriteFatString(bpb.Label, label, LABEL_LENGTH);
    WriteFatString(bpb.FsType, (fatWidth == 12) ? "FAT12" : "FAT16", NAME_LENGTH);

    bool success = FatDisk::CreateNew(path, &bpb, g_nSectorOffset);
    if (!success) {
        LogError("failed to create disk\n");
        return STATUS_ERROR;
    }

    FatDisk *disk = FatDisk::Open(path, g_nSectorOffset);
    if (!disk) {
        LogError("failed to open newly-created disk\n");
        return STATUS_ERROR;
    }

    PrintDiskInfo(path, disk);

    delete disk;
    return STATUS_SUCCESS;
}
