#include "Command.hpp"
#include "FatDisk.hpp"

int Mkdir(const Command *cmd, const CommandArgs *args)
{
    (void) cmd;
    const char *diskPath = NULL;
    const char *dirPath = NULL;

    bool makeParent = 0;

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
            ":+p",
            LongOptions, &optidx);

        if (c == -1) break;
        ProcessGlobalOption(args->Argv, LongOptions, c);

        switch (c) {
            case 'p':
                makeParent = 1;
                break;;
        }
    }

    int pos = 0;
    while (optind < args->Argc) {
        optarg = args->Argv[optind++];
        switch (pos++) {
            case 0:
                diskPath = optarg;
                break;
            case 1:
                dirPath = optarg;
                break;
            default:
                LogError_BadArg(optarg);
                return STATUS_INVALIDARG;
                break;
        }
    }

    CheckParam(diskPath != NULL, "missing disk image file name\n");
    CheckParam(dirPath != NULL, "missing directory name\n");

    bool success = true;
    FatDisk *disk = NULL;

    DirEntry parent;
    DirEntry *pParent = &parent;
    DirEntry *pParentDirTable = NULL;
    uint32_t parentDirSize;

    DirEntry newDir;
    DirEntry *pNewDir = &newDir;

    char *parentName = NULL;
    char *newDirName = NULL;

    char dirPathCopy[MAX_PATH];
    strncpy(dirPathCopy, dirPath, MAX_PATH);

    disk = FatDisk::Open(diskPath, g_nSectorOffset);
    SafeRIF(disk, "failed to open disk\n");

    SafeRIF(disk->FindFile(pParent, NULL, "/"),
        "failed to locate root directory\n");

    newDirName = strtok(dirPathCopy, "/\\");

    while (true) {
        char *nextName = strtok(NULL, "/\\");
        bool leaf = (nextName == NULL);

        parentDirSize = disk->GetFileAllocSize(pParent);
        pParentDirTable = (DirEntry *) SafeAlloc(parentDirSize);

        SafeRIF(disk->ReadFile((char *) pParentDirTable, pParent),
            "failed to read parent directory\n");

        DirEntry *pTmpDir = NULL;
        bool dirExists = disk->FindFileInDir(&pTmpDir, pParentDirTable,
            parentDirSize, newDirName);

        if (dirExists) {
            // Directory exists, and we are at the leaf node, FAIL.
            SafeRIF(!leaf, "'%s' exists\n", newDirName);
            // TODO: need a better way to log and fail, SafeRIT?
        }
        else {
            // Directory does not exist and we have more to traverse, FAIL.
            SafeRIF(leaf, "directory not found - %s\n", newDirName);
            // TODO: -p
        }

        if (leaf) break;
        SafeRIF(IsDirectory(pTmpDir), "not a directory - %s\n", newDirName);

        *pParent = *pTmpDir;
        parentName = newDirName;
        newDirName = nextName;

        SafeFree(pParentDirTable);
    }

    SafeRIF(disk->CreateDirectory(pNewDir, pParent, newDirName),
        "failed to create directory - %s\n", newDirName);

    // TODO: need to update parent dir entry timestamp.
    // e.g. FOO/BAR exists, we create BAZ in BAR. The following should have
    // identical 'modified' and 'accessed' timestamps:
    //   FOO/BAR/
    //   FOO/BAR/BAZ/
    //   FOO/BAR/BAZ/.
    //   FOO/BAR/BAZ/..

Cleanup:
    SafeFree(pParentDirTable);
    delete disk;
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}
