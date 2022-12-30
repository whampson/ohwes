#include "Command.hpp"
#include "FatDisk.hpp"

int Extract(const Command *cmd, const CommandArgs *args)
{
    (void) cmd;
    const char *diskPath = NULL;
    const char *filePath = NULL;
    const char *outPath = NULL;

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
                diskPath = optarg;
                break;
            case 1:
                filePath = optarg;
                break;
            case 2:
                outPath = optarg;
                break;
            default:
                LogError_BadArg(optarg);
                return STATUS_INVALIDARG;
                break;
        }
    }

    CheckParam(diskPath != NULL, "missing disk image file name\n");
    CheckParam(filePath != NULL, "missing file name\n");
    if (outPath == NULL) {
        outPath = GetFileName(filePath);
    }

    // TODO: --force to allow overwrite
    if (FileExists(outPath)) {
        LogError("%s exists\n", outPath);
        return STATUS_ERROR;
    }

    FatDisk *disk = FatDisk::Open(diskPath, g_nSectorOffset);
    if (!disk) {
        return STATUS_ERROR;
    }

    bool success = true;
    char *fileBuf = NULL;
    FILE *fp = NULL;
    uint32_t fileSize;
    uint32_t allocSize;
    DirEntry f;

    SafeRIF(disk->FindFile(&f, NULL, filePath), "file not found - %s\n", filePath);
    SafeRIF(!IsDeviceFile(&f), "'%s' is a device file\n", filePath);

    allocSize = disk->GetFileAllocSize(&f);
    fileSize = disk->GetFileSize(&f);
    if (fileSize > allocSize) {
        LogWarning("stored file size is larger than file allocation size\n");
    }

    fileBuf = (char *) SafeAlloc(allocSize);
    SafeRIF(disk->ReadFile(fileBuf, &f), "failed to read file - %s\n", filePath);
    SafeRIF(!IsDirectory(&f), "cannot extract a directory (yet...)\n");

    fp = SafeOpen(outPath, "wb", NULL);
    SafeWrite(fp, fileBuf, fileSize);

Cleanup:
    SafeClose(fp);
    SafeFree(fileBuf);
    delete disk;
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}