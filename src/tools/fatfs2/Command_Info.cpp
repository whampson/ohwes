#include "Command.hpp"
#include "FatDisk.hpp"

static void PrintDiskInfo(const char *path, const FatDisk *disk)
{
    const BiosParamBlock *bpb = disk->GetBPB();
    uint32_t sectorCount = (bpb->SectorCount) ? bpb->SectorCount : bpb->SectorCountLarge;
    uint32_t rootSectorCount = CeilDiv(bpb->RootDirCapacity * sizeof(DirEntry), bpb->SectorSize);
    uint32_t dataSectors = sectorCount -
        (bpb->ReservedSectorCount + (bpb->SectorsPerTable * bpb->TableCount) + rootSectorCount);
    uint32_t clusterCount = dataSectors / bpb->SectorsPerCluster;
    uint32_t clusterSize = bpb->SectorsPerCluster * bpb->SectorSize;
    uint32_t bytesTotal = clusterCount * clusterSize;
    uint32_t bytesFree = disk->CountFreeClusters() * clusterSize;
    uint32_t bytesBad = disk->CountBadClusters() * clusterSize;

    assert(sectorCount == disk->GetSectorCount());
    assert(clusterCount == disk->GetClusterCount());

    bool fat12 = clusterCount <= MAX_CLUSTERS_12;

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
        ReadFatString(labelBuf, bpb->Label, LABEL_LENGTH);
        LogInfo("volume ID is %08X", bpb->VolumeId);
        if (labelBuf[0] != '\0')
            LogInfo(", volume label is '%s'\n", labelBuf);
        else
            LogInfo(", volume has no label\n");
    }
    LogInfo("%d bytes free\n", bytesFree);
    LogInfo("%d bytes total\n", bytesTotal);
    if (bytesBad) LogInfo("%d bytes in bad clusters\n", bytesBad);
}

static void PrintFileInfo(const FatDisk *disk, const DirEntry *pFile)
{
    uint32_t index = 0;
    uint32_t cluster = 0;
    time_t now = time(NULL);
    struct tm * timestamp = localtime(&now);
    char dateBuf[MAX_DATE];
    char timeBuf[MAX_TIME];
    char shortNameBuf[MAX_SHORTNAME];
    char lineBuf[80];
    char *ptr;

    GetShortName(shortNameBuf, pFile);

    LogInfo("%s statistics:\n", shortNameBuf);
    LogInfo(" File size: %u %s\n",
        PluralForPrintf(disk->GetFileSize(pFile), "byte"));
    LogInfo("Alloc size: %u bytes (%d %s)\n",
        disk->GetFileAllocSize(pFile),
        PluralForPrintf(disk->CountClusters(pFile), "cluster"));

    GetCreationTime(timestamp, pFile);
    FormatDate(dateBuf, timestamp);
    FormatTime(timeBuf, timestamp);
    LogInfo("   Created: %s %s\n", dateBuf, timeBuf);

    GetModifiedTime(timestamp, pFile);
    FormatDate(dateBuf, timestamp);
    FormatTime(timeBuf, timestamp);
    LogInfo("  Modified: %s %s\n", dateBuf, timeBuf);

    GetAccessedTime(timestamp, pFile);
    FormatDate(dateBuf, timestamp);
    LogInfo("  Accessed: %s\n", dateBuf);

    LogInfo("Attributes: ");
    if (HasAttribute(pFile, ATTR_READONLY)) LogInfo("Read-Only,");
    if (HasAttribute(pFile, ATTR_HIDDEN))   LogInfo("Hidden, ");
    if (HasAttribute(pFile, ATTR_SYSTEM))   LogInfo("System, ");
    if (HasAttribute(pFile, ATTR_DIRECTORY))LogInfo("Directory, ");
    if (HasAttribute(pFile, ATTR_DEVICE))   LogInfo("Device File, ");
    if (HasAttribute(pFile, ATTR_ARCHIVE))  LogInfo("Achive, ");

    LogInfo("\n  Clusters:\n");
    cluster = pFile->FirstCluster;
    index = 0;

    ptr = lineBuf;
    ptr += sprintf(lineBuf, "    ");

    do {
        if (index != 0 && (index % 8) == 0) {
            LogInfo("%s\n", lineBuf);
            ptr = lineBuf;
            ptr += sprintf(lineBuf, "    ");
        }
        ptr += sprintf(ptr, "%04X ", cluster);
        cluster = disk->GetCluster(cluster);
        index++;
    } while (!disk->IsEOC(cluster));

    if ((index % 8) != 0) {
        LogInfo("%s\n", lineBuf);
    }
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

    FatDisk *disk = FatDisk::Open(path, sectorOffset);
    if (!disk) {
        return STATUS_ERROR;
    }

    bool success = true;

    if (file == NULL) {
        PrintDiskInfo(path, disk);
    }
    else {
        DirEntry f;
        const DirEntry *pFile = &f;

        SafeRIF(disk->FindFile(&f, file), "file not found - %s\n", file);

        if (IsRoot(pFile)) {
            PrintDiskInfo(path, disk);
        }
        else {
            PrintFileInfo(disk, pFile);
        }
    }

Cleanup:
    delete disk;
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}
