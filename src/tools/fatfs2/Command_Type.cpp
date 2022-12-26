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

    FatDisk *disk = FatDisk::Open(path, sectorOffset);
    if (!disk) {
        return STATUS_ERROR;
    }

    bool success = true;
    char *fileBuf = NULL;
    uint32_t size;
    uint32_t allocSize;
    DirEntry f;

    if (file == NULL) {
        file = "/";
    }

    SafeRIF(disk->FindFile(&f, file), "file not found - %s\n", file);
    SafeRIF(!IsDeviceFile(&f), "'%s' is a device file\n", file);

    allocSize = disk->GetFileAllocSize(&f);
    size = disk->GetFileSize(&f);
    if (size > allocSize) {
        LogWarning("stored file size is larger than file allocation size\n");
    }

    fileBuf = (char *) SafeAlloc(allocSize+1);  // +1 to account for added NUL
    SafeRIF(disk->ReadFile(fileBuf, &f), "failed to read file - %s\n", file);

    if (IsDirectory(&f)) {
        uint32_t count = size / sizeof(DirEntry);
        const DirEntry *e = (DirEntry *) fileBuf;

        for (uint32_t i = 0; i < count; i++, e++) {
            if (!IsValidFile(e))
                continue;
            char shortName[MAX_SHORTNAME];
            GetShortName(shortName, e);
            LogInfo("%s\n", shortName);
        }
    }
    else {
        fileBuf[size] = '\0';
        LogInfo("%s\n", fileBuf);
    }

Cleanup:
    SafeFree(fileBuf);
    delete disk;
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}