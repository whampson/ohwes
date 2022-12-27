#include "Command.hpp"
#include "FatDisk.hpp"

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

    if (file == NULL) {
        // Disk info
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

        delete disk;
        return STATUS_SUCCESS;
    }

    bool success = true;

    DirEntry f;
    bool found = disk->FindFile(&f, file);
    if (!found) {
        LogError("file not found - %s\n", file);
        success = false;
        goto Cleanup;
    }

    LogInfo("file size = %d\n", f.FileSize);
    // TODO: finish

Cleanup:
    delete disk;
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}
