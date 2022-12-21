#include "Command.hpp"
#include "FatDisk.hpp"

#define CheckParam(x,...) if (!(x)) { LogError(__VA_ARGS__); return STATUS_INVALIDARG; }

static const Command s_pCommands[] = {
    { Create,   // similar to mkdosfs
        "create", PROG_NAME " create [OPTIONS] DISK [NSECTORS]",
        "Create a new FAT file system on the specified disk",
        "  -d NUMBER         Set the drive number to NUMBER\n"
        "  -f COUNT          Create COUNT file allocation tables\n"
        "  -F WIDTH          Select the FAT width (12, or 16)\n"
        "  -g HEADS/SPT      Select the disk geometry (as heads/sectors_per_track)\n"
        "  -i VOLID          Set the volume ID to VOLID (as a 32-bit hex number)\n"
        "  -l LABEL          Set the volume label to LABEL (11 chars max)\n"
        "  -m TYPE           Set the media type ID to TYPE\n"
        "  -r COUNT          Create space for at least COUNT root directory entries\n"
        "  -R COUNT          Create COUNT reserved sectors\n"
        "  -s COUNT          Set the number of logical sectors per cluster to COUNT\n"
        "  -S SIZE           Set the logical sector size to SIZE (power of 2, minimum 512)\n"
        "  --force           Overwrite the disk image file if it already exists\n"
        "  --no-align        Disable structure alignment\n"
        "  --offset=SECTOR   Write the file system to a specific sector on disk\n"
    },
    { Help,
        "help", PROG_NAME " help [COMMAND]",
        "Print the help menu for this tool or a specific command",
        NULL
    },
    { Info,
        "info", PROG_NAME " info [OPTIONS] DISK [FILE]",
        "Print information about a disk or a file on disk",
        "  --offset=SECTOR   Read the file system from a specific sector on disk\n"
    }
};

const Command * GetCommands()
{
    return s_pCommands;
}

int GetCommandCount()
{
    return (int) (sizeof(s_pCommands) / sizeof(Command));
}

const Command * FindCommand(const char *name)
{
    for (int i = 0; i < GetCommandCount(); i++) {
        if (strcmp(s_pCommands[i].Name, name) == 0) {
            return &s_pCommands[i];
        }
    }

    return NULL;
}

int PrintCommandHelp(const Command *cmd)
{
    if (cmd->Synopsis)
        printf("Usage: %s\n", cmd->Synopsis);

    if (cmd->Description)
        printf("%s.\n", cmd->Description);

    if (cmd->Options)
        printf("\nOptions:\n%s\n", cmd->Options);

    return STATUS_SUCCESS;
}

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
    int sectorOffset = 0;

    static struct option LongOptions[] = {
        GLOBAL_LONGOPTS,
        { "force", no_argument, &force, 1 },
        { "no-align", no_argument, &noAlign, 1 },
        { "offset", required_argument, 0, 'o' },
        { 0, 0, 0, 0 }
    };

    optind = 0;     // getopt: reset option index
    opterr = 0;     // getopt: prevent default error messages

    // Parse option arguments
    while (true) {
        int optIdx = 0;
        int c = getopt_long(
            args->Argc, args->Argv,
            GLOBAL_OPTSTRING "d:f:F:g:i:l:m:r:R:s:S:",
            LongOptions, &optIdx);

        if (ProcessGlobalOption(c)) {
            return STATUS_SUCCESS;
        }
        if (c == -1) break;

        switch (c) {
            case 0: // long option
                if (LongOptions[optIdx].flag != 0)
                    break;
                assert(!"unhandled getopt_long() case: non-flag long option");
                break;
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
            case 'o':
                sectorOffset = (unsigned int) strtol(optarg, NULL, 0);
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
            case '?':
                if (optopt != 0)
                    LogError_BadOpt(optopt);
                else
                    LogError_BadLongOpt(&args->Argv[optind - 1][2]);
                return STATUS_INVALIDARG;
                break;
            case ':':
                if (optopt != 0)
                    LogError_MissingOptArg(optopt);
                else
                    LogError_MissingLongOptArg(&args->Argv[optind - 1][2]);
                return STATUS_INVALIDARG;
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

    CheckParam(path != NULL, "missing disk image file name\n")
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
    FILE *fp = fopen(path, "r");
    if (fp != NULL) {
        if (!force) {
            LogError("%s exists\n", path);
            fclose(fp);
            return STATUS_ERROR;
        }
        fclose(fp);
    }

    if (sectorCount <= 4096) {
        noAlign = 1;
        LogVerbose("disabling alignment for small disk\n");
    }

    if (!noAlign) {
        sectorCount = Align(sectorCount, sectorsPerCluster);
        rootDirCapacity = RoundUp(rootDirCapacity, sectorSize / sizeof(DirEntry));
    }

    int rootSectorCount = Ceiling(rootDirCapacity * sizeof(DirEntry), sectorSize);
    int sectorsUsed = rootSectorCount + reservedSectorCount;

    int fatSize = 0;
    int clusters = 0;
    bool fatSizeKnown = false;

    // Figure out how big the FAT needs to be to address all clusters on disk.
    do {
        sectorsUsed += fatCount;
        fatSize += sectorSize;

        int sectorsUsedAligned = Align(sectorsUsed, sectorsPerCluster);
        clusters = (sectorCount - sectorsUsedAligned) / sectorsPerCluster;

        int fatCapacity12 = ((fatSize / 3) * 2) - FIRST_CLUSTER;
        int fatCapacity16 = (fatSize / 2) - FIRST_CLUSTER;

        bool maybeFat12 = (fatWidth == 0 || fatWidth == 12);
        bool maybeFat16 = (fatWidth == 0 || fatWidth == 16);

        if (clusters > MAX_CLUSTER_12 && fatCapacity12 > MAX_CLUSTER_12) {
            // TODO: we could squeeze extra clusters out of the 12-bit FAT
            // by properly handling the sector boundaries
            if (fatWidth == 12) {
                LogError("too many clusters for FAT12\n");
                return STATUS_ERROR;
            }
            maybeFat12 = false;
        }

        if (clusters > MAX_CLUSTER_16 && fatCapacity16 > MAX_CLUSTER_16) {
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
                LogVerbose("selecting FAT12 because %d < %d clusters\n", clusters, MIN_CLUSTER_16);
            }
            if (clusters < MIN_CLUSTER_12)  {
                LogError("not enough clusters for FAT12\n");
                return STATUS_ERROR;
            }
            fatWidth = 12;
            fatSizeKnown = true;
        }
        else if (maybeFat16 && clusters <= fatCapacity16) {
            if (fatWidth == 0 && clusters >= MIN_CLUSTER_16) {
                LogVerbose("selecting FAT16 because %d >= %d clusters\n", clusters, MIN_CLUSTER_16);
            }
            if (fatWidth == 16 && clusters < MIN_CLUSTER_16) {
                LogError("not enough clusters for FAT16\n");
                return STATUS_ERROR;
            }
            fatWidth = 16;
            fatSizeKnown = true;
        }
    } while (!fatSizeKnown);

    // Create the BPB
    BiosParamBlock bpb;
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

    SetLabel(bpb.Label, label);
    SetName(bpb.FsType, (fatWidth == 12) ? "FAT12" : "FAT16");

    bool success = FatDisk::CreateNew(path, &bpb, sectorOffset);
    if (!success) {
        LogError("failed to create disk\n");
        return STATUS_ERROR;
    }

    char labelBuf[MAX_LABEL];
    GetLabel(labelBuf, bpb.Label);

    LogInfo("%s statistics:\n", GetFileName(path));
    LogInfo("%d %s, %d %s, %d %s per track\n",
        PluralForPrintf(sectorCount, "sector"),
        PluralForPrintf(bpb.HeadCount, "head"),
        PluralForPrintf(bpb.SectorsPerTrack, "sector"));
    LogInfo("sector size is %d bytes, %d %s per cluster\n",
        bpb.SectorSize,
        PluralForPrintf(bpb.SectorsPerCluster, "sector"));
    LogInfo("%d reserved %s\n",
        PluralForPrintf(bpb.ReservedSectorCount, "sector"));
    LogInfo("media type is 0x%02X, drive number is 0x%02X\n",
        bpb.MediaType, bpb.DriveNumber);
    LogInfo("%d %d-bit %s, %d %s per FAT, providing %d clusters\n",
        bpb.TableCount, fatWidth,
        Plural(bpb.TableCount, "FAT"),
        PluralForPrintf(fatSize / bpb.SectorSize, "sector"),
        clusters);
    LogInfo("root directory contains %d %s, occupying %d %s\n",
        PluralForPrintf(rootDirCapacity, "slot"),
        PluralForPrintf(rootSectorCount, "sector"));
    LogInfo("volume ID is %08X", bpb.VolumeId);
    if (labelBuf[0] != '\0')
        LogInfo(", volume label is '%s'\n", labelBuf);
    else
        LogInfo(", volume has no label\n");
    LogInfo("%d bytes free\n",
        clusters * sectorsPerCluster * sectorSize);

    return STATUS_SUCCESS;
}

int Help(const Command *cmd, const CommandArgs *args)
{
    if (args->Argc < 2) {
        PrintGlobalHelp();
        return STATUS_SUCCESS;
    }

    const Command *cmdToGetHelpAbout = FindCommand(args->Argv[1]);
    if (!cmdToGetHelpAbout) {
        LogError_BadCommand(args->Argv[1]);
        return STATUS_ERROR;
    }

    PrintCommandHelp(cmdToGetHelpAbout);
    if (cmdToGetHelpAbout == cmd) {
        // Special case for 'help' command to print all valid commands
        const Command *cmds = GetCommands();
        int count = GetCommandCount();
        printf("\nCommands:\n");
        for (int i = 0; i < count; i++) {
            printf("  %-18s%s\n", cmds[i].Name, cmds[i].Description);
        }
        printf("\n");
    }
    return STATUS_SUCCESS;
}

int Info(const Command *cmd, const CommandArgs *args)
{
    (void) cmd;
    const char *path = NULL;
    const char *file = NULL;

    int sectorOffset = 0;

    static struct option LongOptions[] = {
        GLOBAL_LONGOPTS,
        { "offset", required_argument, 0, 'o' },
        { 0, 0, 0, 0 }
    };

    optind = 0;     // getopt: reset option index
    opterr = 0;     // getopt: prevent default error messages

    // Parse option arguments
    while (true) {
        int optIdx = 0;
        int c = getopt_long(
            args->Argc, args->Argv,
            GLOBAL_OPTSTRING,
            LongOptions, &optIdx);

        if (ProcessGlobalOption(c)) {
            return STATUS_SUCCESS;
        }
        if (c == -1) break;

        switch (c) {
            case 'o':
                sectorOffset = (unsigned int) strtol(optarg, NULL, 0);
                break;
            case 0: // long option
                if (LongOptions[optIdx].flag != 0)
                    break;
                assert(!"unhandled getopt_long() case: non-flag long option");
                break;
            case '?':
                if (optopt != 0)
                    LogError_BadOpt(optopt);
                else
                    LogError_BadLongOpt(&args->Argv[optind - 1][2]);
                return STATUS_INVALIDARG;
                break;
            case ':':
                if (optopt != 0)
                    LogError_MissingOptArg(optopt);
                else
                    LogError_MissingLongOptArg(&args->Argv[optind - 1][2]);
                return STATUS_INVALIDARG;
                break;
        }
    }

    int pos = 0;
    while (optind < args->Argc) {
        optarg = args->Argv[optind++];
        switch (pos++) {
            case 0:
                path = optarg;
                break;
            case 1:
                file = optarg;
                break;
            default:
                LogError_BadArg(optarg);
                return STATUS_INVALIDARG;
                break;
        }
    }

    CheckParam(path != NULL, "missing disk image file name\n");

    (void) file;

    FatDisk *disk = FatDisk::Open(path, sectorOffset);
    if (!disk) {
        return STATUS_ERROR;
    }

    if (file != NULL) {
        // File info
        // TODO
        delete disk;
        return STATUS_SUCCESS;
    }

    // Disk info
    const BiosParamBlock *bpb = disk->GetBPB();
    int sectorCount = (bpb->SectorCount) ? bpb->SectorCount : bpb->SectorCountLarge;
    int rootSectorCount = Ceiling(bpb->RootDirCapacity * sizeof(DirEntry), bpb->SectorSize);
    int dataSectors = sectorCount - (bpb->ReservedSectorCount + (bpb->SectorsPerTable * bpb->TableCount) + rootSectorCount);
    int clusterCount = dataSectors / bpb->SectorsPerCluster;
    int bytesTotal = clusterCount * bpb->SectorsPerCluster * bpb->SectorSize;
    int bytesFree = disk->CountFreeClusters() * bpb->SectorsPerCluster * bpb->SectorSize;
    int bytesBad = disk->CountBadClusters() * bpb->SectorsPerCluster * bpb->SectorSize;

    assert(sectorCount == disk->GetSectorCount());
    assert(clusterCount == disk->GetClusterCount());

    bool fat12 = clusterCount <= MAX_CLUSTER_12;

    LogInfo("%s statistics:\n", GetFileName(path));
    LogInfo("%d %s, %d %s, %d %s per track\n",
        PluralForPrintf(sectorCount, "sector"),
        PluralForPrintf(bpb->HeadCount, "head"),
        PluralForPrintf(bpb->SectorsPerTrack, "sector"));
    LogInfo("sector size is %d bytes, %d %s per cluster\n",
        bpb->SectorSize,
        PluralForPrintf(bpb->SectorsPerCluster, "sector"));
    LogInfo("%d reserved %s\n",
        PluralForPrintf(bpb->ReservedSectorCount, "sector"));
    LogInfo("media type is 0x%02X, drive number is 0x%02X\n",
        bpb->MediaType, bpb->DriveNumber);
    LogInfo("%d %d-bit %s, %d %s per FAT, providing %d clusters\n",
        bpb->TableCount, fat12 ? 12 : 16,
        Plural(bpb->TableCount, "FAT"),
        PluralForPrintf(bpb->SectorsPerTable, "sector"),
        clusterCount);
    LogInfo("root directory contains %d %s, occupying %d %s\n",
        PluralForPrintf(bpb->RootDirCapacity, "slot"),
        PluralForPrintf(rootSectorCount, "sector"));
    if (bpb->Signature == BPBSIG_DOS41) {
        char labelBuf[MAX_LABEL];
        GetLabel(labelBuf, bpb->Label);
        LogInfo("volume ID is %08X", bpb->VolumeId);
        if (labelBuf[0] != '\0')
            LogInfo(", volume label is '%s'\n", labelBuf);
        else
            LogInfo(", volume has no label\n");
    }
    LogInfo("%d bytes total\n", bytesTotal);
    LogInfo("%d bytes free\n", bytesFree);
    if (bytesBad) LogInfo("%d bytes in bad clusters\n", bytesBad);

    delete disk;
    return STATUS_SUCCESS;
}
