#include "Command.hpp"
#include "FatDisk.hpp"

int Touch(const Command *cmd, const CommandArgs *args)
{
    (void) cmd;
    const char *path = NULL;
    const char *file = NULL;

    int accTime = 1;
    int modTime = 1;

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
            ":+am",
            LongOptions, &optidx);

        if (c == -1) break;
        ProcessGlobalOption(args->Argv, LongOptions, c);

        switch (c) {
            case 'a':
                modTime = 0;
                break;
            case 'm':
                accTime = 0;
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

    FatDisk *disk = FatDisk::Open(path, g_nSectorOffset);
    if (!disk) {
        return STATUS_ERROR;
    }

    bool success = true;
    DirEntry f;
    time_t t;
    struct tm tm;

    if (file == NULL) {
        file = "/";
    }

    SafeRIF(disk->FindFile(&f, NULL, file), "file not found - %s\n", file);
    SafeRIF(!IsRoot(&f), "cannot touch root directory because it does not have a timestamp\n");

    t = time(NULL);
    localtime_r(&t, &tm);

    if (modTime) {
        SetModifiedTime(&f, &tm);
    }
    if (accTime) {
        SetAccessedTime(&f, &tm);
    }

    // TODO: write



Cleanup:
    delete disk;
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}
