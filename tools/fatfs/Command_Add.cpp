#include "Command.hpp"
#include "FatDisk.hpp"

static const char *s_diskPath = NULL;
static const char *s_srcFilePath = NULL;
static const char *s_dstFilePath = NULL;
static int s_bForce = 0;

static bool ParseArgs(const CommandArgs *args)
{
    static struct option LongOptions[] = {
        GLOBAL_LONGOPTS,
        { "force", no_argument, &s_bForce, 1 },
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
                s_diskPath = optarg;
                break;
            case 1:
                s_srcFilePath = optarg;
                break;
            case 2:
                s_dstFilePath = optarg;
                break;
            default:
                LogError_BadArg(optarg);
                return false;
                break;
        }
    }

    CheckParam(s_diskPath != NULL, "missing disk image file name\n");
    CheckParam(s_srcFilePath != NULL, "missing source file name\n");

    if (s_dstFilePath == NULL) {
        s_dstFilePath = GetFileName(s_srcFilePath);     // will add to root
    }

    return true;
}

int Add(const Command *cmd, const CommandArgs *args)
{
    (void) cmd;

    if (!ParseArgs(args)) {
        return STATUS_INVALIDARG;
    }

    bool success = true;

    FatDisk *disk = NULL;
    FILE *fp = NULL;

    size_t fileSize;
    uint32_t dirSize;

    DirEntry f;
    DirEntry p;
    DirEntry *pFileDesc = &f;
    DirEntry *pParent = &p;
    DirEntry *pParentDir = NULL;
    char *pFileBuf = NULL;
    bool exists;

    disk = FatDisk::Open(s_diskPath, g_nSectorOffset);
    SafeRIF(disk, "failed to open disk\n");

    fp = SafeOpen(s_srcFilePath, "rb", &fileSize);
    SafeRIF(fileSize <= UINT32_MAX, "file is too large!\n");

    // TODO: check if srcFilePath is a directory, etc.
    SafeRIF(FileExists(s_srcFilePath), "file not found - %s\n", s_srcFilePath);

    exists = disk->FindFile(pFileDesc, pParent, s_dstFilePath);
    if (exists && !s_bForce) {
        SafeRIF(0, "'%s' exists\n", s_dstFilePath);
    }
    // TODO: verify that this is guaranteed to get the parent even when the file
    // doesn't exist
    // TODO: what if the parent doesn't exist?

    // TODO: what if we need to add a new cluster to the dir?
    // TODO: what if we have the root?

    // TODO: CreateFile(path, overwrite, data)


    dirSize = disk->GetFileAllocSize(pParent);
    pParentDir = (DirEntry *) SafeAlloc(dirSize);
    SafeRIF(disk->ReadFile((char *) pParentDir, pParent),
        "failed to read parent directory\n");

    if (exists)
    {
        SafeRIF(disk->FindFileInDir(&pFileDesc, pParentDir, dirSize, GetFileName(s_dstFilePath)),
            "failed to locate file in directory\n");
        assert(memcmp(pFileDesc, &f, sizeof(DirEntry)) == 0);
    }
    else
    {
        // search for next available slot in dir table
        // TODO: add sector to chain if full (or error if root)
        pFileDesc = pParentDir;
        while (!IsFree(pFileDesc)) {
            pFileDesc++;
            uint32_t slotsLeft = (dirSize / sizeof(DirEntry)) - (uint32_t) (pFileDesc - pParentDir);
            SafeRIF(slotsLeft > 0, "directory is full!\n");
        }
        InitDirEntry(pFileDesc);
        pFileDesc->FirstCluster = disk->FindNextFreeCluster();
        // TODO: derive valid short name and set long name
    }

    // TODO: fn
    {
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);

        pFileBuf = (char *) SafeAlloc(fileSize);
        SafeRead(fp, pFileBuf, fileSize);
        // TODO: read/write file in chunks? Might be better for very large files

        SetCreationTime(pFileDesc, tm);
        SetModifiedTime(pFileDesc, tm);
        SetAccessedTime(pFileDesc, tm);
        SafeRIF(SetShortName(pFileDesc, GetFileName(s_dstFilePath)), "invalid short name\n");
        pFileDesc->FileSize = fileSize;

        SafeRIF(disk->WriteFile(pFileDesc, pFileBuf),
            "failed to write file\n");
        SafeRIF(disk->WriteFile(pParent, (const char *) pParentDir),
            "failed to write directory\n");
    }

Cleanup:
    SafeFree(pFileBuf);
    SafeFree(pParentDir);
    SafeClose(fp);
    delete disk;
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}
