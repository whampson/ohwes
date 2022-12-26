#include "Command.hpp"
#include "FatDisk.hpp"

int Type(const Command *cmd, const CommandArgs *args)
{
    (void) cmd;
    const char *path = NULL;
    const char *file = NULL;

    bool showAll = false;
    bool showAttr = false;
    bool bareFormat = false;
    bool shortNamesOnly = false;
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
            GLOBAL_OPTSTRING "aAbn",
            LongOptions, &optIdx);

        if (ProcessGlobalOption(c)) {
            return STATUS_SUCCESS;
        }
        if (c == -1) break;

        switch (c) {
            case 'a':
                showAll = true;
                break;
            case 'A':
                showAttr = true;
                break;
            case 'b':
                bareFormat = true;
                break;
            case 'n':
                shortNamesOnly = true;
                break;
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
    CheckParam(file != NULL, "missing file name\n");

    FatDisk *disk = FatDisk::Open(path, sectorOffset);
    if (!disk) {
        return STATUS_ERROR;
    }

    bool success = true;
    char *fileBuf = NULL;
    uint32_t size;
    uint32_t allocSize;
    DirEntry f;

    SafeRIF(disk->FindFile(&f, file), "file not found - %s\n", file);
    SafeRIF(!IsDirectory(&f), "'%s' is a directory\n", file);
    SafeRIF(!IsLabel(&f) && !IsDeviceFile(&f), "not a file - %s\n", file);

    // TODO: if directory, list files?

    size = disk->GetFileSize(&f);
    allocSize = disk->GetFileAllocSize(&f);

    fileBuf = (char *) SafeAlloc(allocSize);
    SafeRIF(disk->ReadFile(fileBuf, &f), "failed to read file - %s\n", file);

    LogInfo("%*s\n", size, fileBuf);

Cleanup:
    SafeFree(fileBuf);
    delete disk;
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}