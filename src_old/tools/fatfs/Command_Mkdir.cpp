#include "Command.hpp"
#include "FatDisk.hpp"

// static bool CreateDirectory(const FatDisk *disk, DirEntry *pDir, DirEntry *pParent, const char *name)

int Mkdir(const Command *cmd, const CommandArgs *args)
{
    (void) cmd;
    const char *diskPath = NULL;
    const char *dirPath = NULL;

    bool makeParent = 0;
    (void) makeParent;

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

    DirEntry newFile;
    DirEntry parent;
    DirEntry *pNewDirTable = NULL;
    DirEntry *pParentDirTable = NULL;
    uint32_t parentDirSize;
    uint32_t parentDirCount;
    uint32_t newDirSize;
    bool fileFound = false;
    DirEntry *e;

    disk = FatDisk::Open(diskPath, g_nSectorOffset);
    SafeRIF(disk, "failed to open disk\n");

    SafeRIF(disk->CreateFile(&newFile, &parent, dirPath),
        "failed to create directory file\n");

    parentDirSize = disk->GetFileAllocSize(&parent);
    parentDirCount = parentDirSize / sizeof(DirEntry);

    newDirSize = disk->GetClusterSize();
    pNewDirTable = (DirEntry *) SafeAlloc(newDirSize);
    pParentDirTable = (DirEntry *) SafeAlloc(parentDirSize);

    memset(pNewDirTable, 0, newDirSize);

    SafeRIF(disk->ReadFile((char *) pParentDirTable, &parent),
        "failed to read parent directory\n");

    e = pParentDirTable;
    for (uint32_t i = 0; i < parentDirCount; i++, e++) {
        if (IsFree(e)) continue;
        if (memcmp(&newFile, e, sizeof(DirEntry)) == 0) {
            fileFound = true;
            break;
        }
    }

    SafeRIF(fileFound, "could not locate file\n");
    // TODO: read file back, set ATTR_DIRECTORY, add . and .. entries, write-out

    SetAttribute(e, ATTR_DIRECTORY);
    e->FirstCluster = disk->FindNextFreeCluster();

    pNewDirTable[0] = *e;
    SetLabel(&pNewDirTable[0], ".");
    pNewDirTable[1] = parent;
    SetLabel(&pNewDirTable[1], "..");
    // TODO: timestamps

    SafeRIF(disk->WriteFile(e, (const char *) pNewDirTable),
        "failed to write directory\n");

    SafeRIF(disk->WriteFile(&parent, (const char *) pParentDirTable),
        "failed to write directory\n");

Cleanup:
    SafeFree(pParentDirTable);
    SafeFree(pNewDirTable);

    delete disk;
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}
