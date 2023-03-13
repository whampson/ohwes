#include "Command.hpp"
#include "FatDisk.hpp"

int Attr(const Command *cmd, const CommandArgs *args)
{
    (void) cmd;
    const char *path = NULL;
    const char *file = NULL;

    int update = 0;
    int updateArc = -1;
    int updateHid = -1;
    int updateRdo = -1;
    int updateSys = -1;
    int updateDev = -1;
    int updateLab = -1;

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
            "+:aAhHrlLRsSvV",
            LongOptions, &optidx);

        if (c == -1) break;
        ProcessGlobalOption(args->Argv, LongOptions, c);

        switch (c) {
            case 'a':
                update = 1;
                updateArc = 1;
                break;
            case 'A':
                update = 1;
                updateArc = 0;
                break;
            case 'h':
                update = 1;
                updateHid = 1;
                break;
            case 'H':
                update = 1;
                updateHid = 0;
                break;
            case 'l':
                update = 1;
                updateLab = 1;
                break;
            case 'L':
                update = 1;
                updateLab = 0;
                break;
            case 'r':
                update = 1;
                updateRdo = 1;
                break;
            case 'R':
                update = 1;
                updateRdo = 0;
                break;
            case 's':
                update = 1;
                updateSys = 1;
                break;
            case 'S':
                update = 1;
                updateSys = 0;
                break;
            case 'v':
                update = 1;
                updateDev = 1;
                break;
            case 'V':
                update = 1;
                updateDev = 0;
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
    DirEntry p;
    DirEntry *pFile = &f;
    DirEntry *pParent = &p;
    DirEntry *dirTable = NULL;
    uint32_t tableSizeBytes;

    if (file == NULL) {
        file = "/";
    }

    const char *fileName = GetFileName(file);

    SafeRIF(disk->FindFile(pFile, pParent, file), "file not found - %s\n", file);
    SafeRIF(!IsRoot(pFile), "root directory does not have attributes to view\n");

    if (!update) {
        bool lab = IsLabel(pFile);
        bool dev = IsDeviceFile(pFile);
        bool dir = IsDirectory(pFile);
        bool arc = IsArchive(pFile);
        bool sys = IsSystemFile(pFile);
        bool hid = IsHidden(pFile);
        bool rdo = IsReadOnly(pFile);

        char shortNameF[MAX_SHORTNAME];
        char shortNameP[MAX_SHORTNAME];
        GetShortName(shortNameF, pFile);
        GetShortName(shortNameP, pParent);

        char lineBuf[80];
        char *ptr = lineBuf;
        ptr += sprintf(ptr, "%c%c%c%c%c%c%c %s",
            lab ? 'L' : '-',
            dev ? 'V' : '-',
            dir ? 'D' : '-',    // TODO: allow ability to change this?
            arc ? 'A' : '-',
            sys ? 'S' : '-',
            hid ? 'H' : '-',
            rdo ? 'R' : '-',
            shortNameF);

        LogInfo("%s\n", lineBuf);
        goto Cleanup;
    }

    tableSizeBytes = disk->GetFileAllocSize(pParent);
    dirTable = (DirEntry *) SafeAlloc(tableSizeBytes);
    SafeRIF(disk->ReadFile((char *) dirTable, pParent),
        "failed to read directory table\n");

    SafeRIF(disk->FindFileInDir(&pFile, dirTable, tableSizeBytes, fileName),
        "could not find file in dir!\n");

    // sanity check, should match value returned from first call to ReadFile()
    assert(memcmp(pFile, &f, sizeof(DirEntry)) == 0);

    if (updateArc != -1) {
        if (updateArc == 1)
            SetAttribute(pFile, ATTR_ARCHIVE);
        else
            ClearAttribute(pFile, ATTR_ARCHIVE);
    }
    if (updateHid != -1) {
        if (updateHid == 1)
            SetAttribute(pFile, ATTR_HIDDEN);
        else
            ClearAttribute(pFile, ATTR_HIDDEN);
    }
    if (updateRdo != -1) {
        if (updateRdo == 1)
            SetAttribute(pFile, ATTR_READONLY);
        else
            ClearAttribute(pFile, ATTR_READONLY);
    }
    if (updateSys != -1) {
        if (updateSys == 1)
            SetAttribute(pFile, ATTR_SYSTEM);
        else
            ClearAttribute(pFile, ATTR_SYSTEM);
    }
    if (updateDev != -1) {
        if (updateDev == 1)
            SetAttribute(pFile, ATTR_DEVICE);
        else
            ClearAttribute(pFile, ATTR_DEVICE);
    }
    if (updateLab != -1) {
        if (updateLab == 1)
            SetAttribute(pFile, ATTR_LABEL);
        else
            ClearAttribute(pFile, ATTR_LABEL);
    }

    SafeRIF(disk->WriteFile(pParent, (const char *) dirTable),
        "failed to write directory table\n");

Cleanup:
    SafeFree(dirTable);
    delete disk;
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}
