#include "Command.hpp"
#include "FatDisk.hpp"

int Type(const Command *cmd, const CommandArgs *args)
{
    (void) cmd;
    const char *path = NULL;
    const char *file = NULL;

    static struct option LongOptions[] = {
        GLOBAL_LONGOPTS,
        { 0, 0, 0, 0 }
    };

    optind = 0;     // getopt: reset option index
    opterr = 0;     // getopt: prevent default error messages
    optidx = 0;     // reset option index

    // Parse option arguments
    while (true) {
        int c = getopt_long(
            args->Argc, args->Argv,
            "+:",
            LongOptions, &optidx);

        if (c == -1) break;
        ProcessGlobalOption(args->Argv, LongOptions, c);
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

    FatDisk *disk = FatDisk::Open(path, g_nSectorOffset);
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

    SafeRIF(disk->FindFile(&f, NULL, file), "file not found - %s\n", file);
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