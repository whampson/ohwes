#include "Command.hpp"
#include "FatDisk.hpp"

int List(const Command *cmd, const CommandArgs *args)
{
    (void) cmd;
    const char *path = NULL;
    const char *file = NULL;

    bool showAll = false;
    bool showAttr = false;
    bool bareFormat = false;
    bool shortNamesOnly = false;
    bool showAllocSize = false;
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
            GLOBAL_OPTSTRING "aAbns",
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
            case 's':
                showAllocSize = true;
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

    // TODO: split up function
    // TODO: recurse
    // TOOD: show allocation size (-s ?)

    bool success = true;
    char *fileBuf = NULL;
    const DirEntry *e = NULL;
    int dirEntryCount = 0;
    int fileCount = 0;
    int dirCount = 0;
    int numShown = 0;
    size_t bytesFree = 0;
    size_t bytesTotal = 0;
    size_t bytesAllocd = 0;
    size_t diskTotal = 0;
    size_t fileSize = 0;
    size_t fileAllocSize = 0;
    DirEntry f;

    if (file == NULL) {
        file = "/";
    }

    SafeRIF(disk->FindFile(&f, file), "file not found - %s\n", file);
    SafeRIF(!IsDeviceFile(&f), "'%s' is a device file\n", file);

    if (IsDirectory(&f)) {
        uint32_t size = disk->GetFileSize(&f);
        fileBuf = (char *) SafeAlloc(size);

        SafeRIF(disk->ReadFile(fileBuf, &f), "failed to read file - %s\n", file);
        e = (DirEntry *) fileBuf;
        dirEntryCount = size / sizeof(DirEntry);
    }
    else {
        e = &f;
        dirEntryCount = 1;
    }

    for (int i = 0; i < dirEntryCount; i++, e++) {
        if (IsFree(e)) {
            continue;
        }

        const int MaxSizeOrType = 11;   // enough to hold 4294967295 = 2^31

        char lineBuf[512] = { };
        char name[MAX_NAME];
        char ext[MAX_EXTENSION];
        wchar_t fullName[MAX_LONGNAME];
        char shortName[MAX_SHORTNAME];
        char label[MAX_LABEL];
        char modDate[MAX_DATE];
        char modTime[MAX_TIME];
        char sizeOrType[MaxSizeOrType];
        char allocSize[MaxSizeOrType];
        bool hasLfn = false;

        // get the LFN first because it'll move the DirEntry pointer
        if (IsLongFileName(e)) {
            if (shortNamesOnly) {
                continue;
            }

            const DirEntry *next = GetLongName(fullName, e);
            i += (int) (next - e);
            e = next;
            hasLfn = (fullName[0] != L'\0');
        }

        bool rdo = IsReadOnly(e);
        bool hid = IsHidden(e);
        bool sys = IsSystemFile(e);
        bool lab = IsLabel(e);
        bool dir = IsDirectory(e);
        bool arc = IsArchive(e);
        bool dev = IsDeviceFile(e);

        if (!showAll && (hid || lab || sys)) {
            continue;
        }

        ReadFatString(label, e->Label, LABEL_LENGTH);
        ReadFatString(name, e->Label, NAME_LENGTH);
        ReadFatString(ext, &e->Label[NAME_LENGTH], EXTENSION_LENGTH);
        GetShortName(shortName, e);

        struct tm modified = { };
        GetModifiedTime(&modified, e);
        FormatDate(modDate, &modified);
        FormatTime(modTime, &modified);

        if (!hasLfn) {
            mbstowcs(fullName, shortName, MAX_SHORTNAME);
        }

        if (showAllocSize) {
            sprintf(allocSize, "");
        }

        if (dev) {
            sprintf(sizeOrType, "<DEVICE>");
        }
        else if (lab) {
            sprintf(sizeOrType, "<LABEL>");
            mbstowcs(fullName, label, MAX_LABEL);
        }
        else if (dir) {
            sprintf(sizeOrType, "<DIR>");
            dirCount++;
        }
        else {
            fileCount++;
            fileSize = disk->GetFileSize(e);
            bytesTotal += fileSize;
            sprintf(sizeOrType, "%*zu", MaxSizeOrType - 1, fileSize);
            if (showAllocSize) {
                fileAllocSize = disk->GetFileAllocSize(e);
                bytesAllocd += fileAllocSize;
                sprintf(allocSize, "%*zu", MaxSizeOrType - 1, fileAllocSize);
            }
        }

        char *ptr = lineBuf;
        if (showAttr) {
            ptr += sprintf(ptr, "%c%c%c%c%c%c%c ",
                dev ? 'V' : '-',    // probably won't ever show up in a normal disk
                arc ? 'A' : '-',
                dir ? 'D' : '-',
                lab ? 'L' : '-',
                sys ? 'S' : '-',
                hid ? 'H' : '-',
                rdo ? 'R' : '-');
        }

        if (!bareFormat) {
            ptr += sprintf(ptr, "%-*s%-*s  %-*s ",
                (!lab) ? NAME_LENGTH + 1 : LABEL_LENGTH + 1,
                (!lab) ? name : label,
                (!lab) ? EXTENSION_LENGTH : 0,
                (!lab) ? ext : "",
                MaxSizeOrType - 1, sizeOrType);

            if (showAllocSize) {
                ptr += sprintf(ptr, "%-*s ", MaxSizeOrType - 1, allocSize);
            }

            ptr += sprintf(ptr, "%s %s ",
                modDate, modTime);

        }

        LogInfo("%s%ls\n", lineBuf, fullName);
        numShown++;
    }

    if (bareFormat) goto Cleanup;
    SafeRIF(numShown != 0, "file not found - %s\n", file);

    bytesFree = disk->CountFreeClusters() * disk->GetClusterSize();
    diskTotal = disk->GetClusterCount() * disk->GetClusterSize();

    if (showAllocSize) {
        int used = ((diskTotal - bytesFree) * 100) / diskTotal;
        LogInfo("%10u %-5s %10zu bytes\n",
            PluralForPrintf(fileCount, "file"), bytesTotal);
        LogInfo("%10u %-5s %10zu bytes allocated\n",
            PluralForPrintf(dirCount, "dir"), bytesAllocd);
        LogInfo("%16s %10zu bytes free\n", "", bytesFree);
        LogInfo("%16s %10zu total disk space, %3d%% used\n", "", diskTotal, used);
    }
    else {
        LogInfo("%10u %-5s %10zu bytes\n",
            PluralForPrintf(fileCount, "file"), bytesTotal);
        LogInfo("%10u %-5s %10zu bytes free\n",
            PluralForPrintf(dirCount, "dir"), bytesFree);
    }

Cleanup:
    SafeFree(fileBuf);
    delete disk;
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}
