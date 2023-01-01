#include "Command.hpp"
#include "FatDisk.hpp"

int Add(const Command *cmd, const CommandArgs *args)
{
    (void) cmd;
    const char *diskPath = NULL;
    const char *srcFilePath = NULL;
    const char *dstFilePath = NULL;
    const char *dstFileName = NULL;

    int force = 0;

    static struct option LongOptions[] = {
        GLOBAL_LONGOPTS,
        { "force", no_argument, &force, 1 },
        { 0, 0, 0, 0 }
    };

    optind = 0;     // getopt: reset option index
    opterr = 0;     // getopt: prevent default error messages
    optidx = 0;     // reset option index

    // Parse option arguments
    while (true) {
        int c = getopt_long(
            args->Argc, args->Argv,
            ":+",
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
                srcFilePath = optarg;
                break;
            case 2:
                dstFilePath = optarg;
                break;
            default:
                LogError_BadArg(optarg);
                return STATUS_INVALIDARG;
                break;
        }
    }

    CheckParam(diskPath != NULL, "missing disk image file name\n");
    CheckParam(srcFilePath != NULL, "missing source file name\n");

    if (dstFilePath == NULL) {
        dstFilePath = GetFileName(srcFilePath);     // will add to root
    }
    dstFileName = GetFileName(dstFilePath);

    bool success = true;

    FatDisk *disk = NULL;
    FILE *fp = NULL;

    size_t fileSize;
    uint32_t dirSize;

    DirEntry f;
    DirEntry p;
    DirEntry *pFile = &f;
    DirEntry *pParent = &p;
    DirEntry *pDirTable = NULL;
    char *pFileBuf = NULL;
    bool exists;

    disk = FatDisk::Open(diskPath, g_nSectorOffset);
    SafeRIF(disk, "failed to open disk\n");

    fp = SafeOpen(srcFilePath, "rb", &fileSize);
    SafeRIF(fileSize <= UINT32_MAX, "file is too large!\n");

    // TODO: check if srcFilePath is a directory, etc.
    SafeRIF(FileExists(srcFilePath), "file not found - %s\n", srcFilePath);

    exists = disk->FindFile(pFile, pParent, dstFilePath);
    SafeRIF(!(exists && !force), "'%s' exists\n", dstFilePath);
    // TODO: verify that this is guaranteed to get the parent even when the file
    // doesn't exist
    // TODO: what if the parent doesn't exist?

    // TODO: what if we need to add a new cluster to the dir?
    // TODO: what if we have the root?
    dirSize = disk->GetFileAllocSize(pParent);
    pDirTable = (DirEntry *) SafeAlloc(dirSize);
    SafeRIF(disk->ReadFile((char *) pDirTable, pParent),
        "failed to read directory\n");

    // TODO: dont modify state within an assert lol
    assert(disk->FindFileInDir(&pFile, pDirTable, dirSize, dstFileName));
    assert(memcmp(pFile, &f, sizeof(DirEntry)) == 0);

    pFileBuf = (char *) SafeAlloc(fileSize);
    SafeRead(fp, pFileBuf, fileSize);
    // TODO: read/write file in chunks? Might be better for very large files

    if (!exists) {
        InitDirEntry(pFile);
        // TODO: LFN and ~n if dst name is too large
        SafeRIF(SetShortName(pFile, dstFileName), "invalid short name\n");
        // TODO: add to dir table
    }


    // !!! TODO: FIXME

    // SafeRIF(disk->WriteFile(pFile, pFileBuf, (uint32_t) fileSize),
    //     "failed to write file\n");
    // SafeRIF(disk->WriteFile(pParent, (const char *) pDirTable, dirSize),
    //     "failed to write directory\n");

Cleanup:
    SafeFree(pFileBuf);
    SafeClose(fp);
    SafeFree(pDirTable);
    delete disk;
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}
