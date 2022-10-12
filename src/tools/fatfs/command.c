#include "command.h"
#include "image.h"


#define GETOPT_MISSING_OPTARG(c)                                                \
do {                                                                            \
    if (optopt == (c)) {                                                        \
        LogError("missing argument for '%c'\n", optopt);                        \
        return STATUS_INVALIDARG;                                               \
    }                                                                           \
} while (0)

#define GETOPT_INVALID()                                                        \
do {                                                                            \
    if (isprint(optopt)) {                                                      \
        LogError("invalid option - '%c'\n", optopt);                            \
        return STATUS_INVALIDARG;                                               \
    } else {                                                                    \
        LogError("invalid option character - '\\x%02x'\n", optopt);             \
        return STATUS_INVALIDARG;                                               \
    }                                                                           \
} while (0)

static const Command s_pCommands[] =
{
    // { Add,
    //     "add", "add [OPTIONS] SOURCE[...]",
    //     "Add files to the disk image.",
    //     "SOURCE specifies the path to one or more files on the host system.\n"
    //     "Files are placed in the root directory unless -d is supplied.\n"
    //     "\n"
    //     "Options:\n"
    //     "    -d TARGETDIR    Add files to TARGETDIR.\n"
    //     "    -f              Overwrite files if they exist regardless of permissions.\n"
    //     "    -p              Create parent directories if they do not exist.\n"
    // },
    // { Attr,
    //     "attr", "attr [OPTIONS] FILE",
    //     "Change file attributes.",
    //     "Options:\n"
    //     "    -H              Set the 'hidden' bit.\n"
    //     "    -R              Set the 'read-only' bit.\n"
    //     "    -S              Set the 'system file' bit.\n"
    //     "    -h              Clear the 'hidden' bit.\n"
    //     "    -r              Clear the 'read-only' bit.\n"
    //     "    -s              Clear the 'system file' bit.\n"
    //     "    -X VALUE        Set the attributes byte to VALUE.\n"
    // },
    // { Copy,
    //     "copy", "copy [OPTIONS] SOURCE TARGET",
    //     "Copy the contents of a file or directory.",
    //     "Options:\n"
    //     "    -f              Overwrite TARGET if it exists regardless of permissions.\n"
    //     "    -p              Create parent directories if they do not exist.\n"
    //     "    -r              Copy subdirectories.\n"
    // },
    // { Create,
    //     "create", "create [OPTIONS]",
    //     "Create a new FAT-formatted disk image.",
    //     "DISKIMAGE will be used as the path to create the new disk image.\n"
    //     "\n"
    //     "Options:\n"
    //     "    -L              Set the volume label.\n"
    //     "    --force         Overwrite the disk image file if it exists.\n",
    // },
    { Extract,
        "extract", "extract [OPTIONS] SOURCE[...]",
        "Extract files from the disk image.",
        "SOURCE specifies the path to one or more files on the disk image.\n"
        "Files are placed in the current working directory unless -d is supplied.\n"
        "\n"
        "Options:\n"
        "    -d TARGETDIR    Extract files to TARGETDIR.\n"
        "    -f              Overwrite destination files if they exist.\n"
        "    -r              Extract subdirectories.\n"
    },
    { Info,
        "info", "info [FILE]",
        "Print file, directory, or disk information.",
        NULL,
    },
    { List,
        "list", "list [OPTIONS] [FILE]",
        "List the contents of a directory (the root directory by default).",
        "Options:\n"
        "  -a       List all files; include hidden files.\n"
        "  -b       Bare format; print file names only.\n"
        "  -n       Use short names only.\n"
        // "  -r       Print the contents of subdirectories.\n"     // TODO
    },
    // { Mkdir,
    //     "mkdir", "mkdir [OPTIONS] PATH",
    //     "Create a directory.",
    //     "Options:\n"
    //     "    -p              Create parent directories if they do not exist.\n"
    // },
    // { Move,
    //     "move", "move [OPTIONS] SOURCE TARGET",
    //     "Move a file or directory to a new location.",
    //     "Options:\n"
    //     "    -f              Overwrite TARGET if it exists regardless of permissions.\n"
    //     "    -p              Create parent directories if they do not exist.\n"
    // },
    // { Remove,
    //     "remove", "remove [OPTIONS] FILE",
    //     "Remove a file or directory.",
    //     "Options:\n"
    //     "    -f              Remove PATH regardless of permissions or directory\n"
    //     "                    contents.\n"
    // },
    // { Rename,
    //     "rename", "rename FILE NEWNAME",
    //     "Rename a file or directory.",
    //     NULL,
    // },
    // { Touch,
    //     "touch", "touch [OPTIONS] FILE",
    //     "Change file access and modification times.",
    //     "By default, both the access and modification times are updated to the current\n"
    //     "date and time. If FILE does not exist, it will be created as an empty file.\n"
    //     "\n"
    //     "Options:\n"
    //     "    -a              Change the access time. Modification time is not updated\n"
    //     "                    unless -m is specified.\n"
    //     "    -m              Change the modification time. Access time is not updated\n"
    //     "                    unless -a is specified.\n"
    //     "    -d MMDDYYYY     Set access and/or modification date to MMDDYYYY.\n"
    //     "                        MM      Month, from 01 to 12.\n"
    //     "                        DD      Day, from 01 to 31.\n"
    //     "                        YYYY    Year, from 1980 to 2107.\n"
    //     "    -t hh:mm:ss     Set access and/or modification time to hh:mm:ss.\n"
    //     "                        hh      Hours, from 00 to 23.\n"
    //     "                        mm      Minutes, from 00 to 59.\n"
    //     "                        ss      Seconds, from 00 to 59.\n"
    //     "    -f              Update access and modification times regardless of\n"
    //     "                    permissions.\n"
    // },
    { Type,
        "type", "type FILE",
        "Print the contents of a file.",
        NULL,
    }
};

const Command * GetCommands(void)
{
    return s_pCommands;
}

int GetCommandCount(void)
{
    return (int) (sizeof(s_pCommands) / sizeof(Command));
}

const Command * FindCommand(const char *name)
{
    for (int i = 0; i < GetCommandCount(); i++)
    {
        if (strcmp(s_pCommands[i].Name, name) == 0)
        {
            return &s_pCommands[i];
        }
    }

    return NULL;
}

int Add(const Command *cmd, const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Attr(const Command *cmd, const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Create(const Command *cmd, const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Copy(const Command *cmd, const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Extract(const Command *cmd, const CommandArgs *args)
{
    bool success = true;
    char shortname[MAX_SHORTNAME];
    char *buf = NULL;
    FILE *fp = NULL;
    DirEntry file;

    if (args->Argc < 2)
    {
        LogError("missing file\n");
        return STATUS_INVALIDARG;
    }

    RIF(OpenImage(args->ImagePath));

    for (int i = 1; i < args->Argc; i++)
    {
        const char *path = args->Argv[i];

        success = FindFile(&file, path);
        if (!success)
        {
            LogError("file not found - %s\n", path);
            RIF(false);
        }

        size_t size = GetFileSize(&file);
        buf = SafeAlloc(size);
        ReadFile(buf, &file);

        if (IsDirectory(&file))
        {
            LogError("%s is a directory.\n", path);
            success = false;
            goto Cleanup;

            // TODO: extract entire dir? recurse?

            // const DirEntry *e = (const DirEntry *) buf;
            // int count = size / sizeof(DirEntry);

            // for (int i = 0; i < count; i++, e++)
            // {
            //     if (!IsFile(e))
            //     {
            //         // Skip free/deleted slots, LFNs, volume labels
            //         continue;
            //     }

            //     GetShortName(shortname, e);
            //     printf("%s\n", shortname);
            // }
        }

        // TODO: honor -d, -f, -r

        // TODO: get long name
        char shortname[MAX_SHORTNAME];
        GetShortName(shortname, &file);

        fp = SafeOpen(shortname, "wb");
        SafeWrite(fp, buf, size);
        SafeClose(fp);

        SafeFree(buf);
    }

Cleanup:
    SafeFree(buf);
    CloseImage();
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}

int Info(const Command *cmd, const CommandArgs *args)
{
    bool success = true;
    char *buf = NULL;

    RIF(OpenImage(args->ImagePath));

    if (args->Argc < 2)
    {
        //
        // Disk image info
        //

        const BiosParamBlock *bpb = GetBiosParams();

        char label[LABEL_LENGTH + 1] = { 0 };
        strncpy(label, bpb->Label, LABEL_LENGTH);

        char fsId[NAME_LENGTH + 1];
        GetName(fsId, bpb->FileSystemType);

        int sectorSize = bpb->SectorSize;
        int sectorCount = bpb->SectorCount;
        int sectorsPerCluster = bpb->SectorsPerCluster;
        int clusterSize = sectorSize * sectorsPerCluster;
        int reservedSectors = bpb->ReservedSectorCount;
        int numTables = bpb->TableCount;
        int tableSectors = numTables * bpb->SectorsPerTable;
        int maxNumRootEntries = bpb->MaxRootDirEntryCount;
        int rootSize = maxNumRootEntries * sizeof(DirEntry);
        int numRootSectors = rootSize / sectorSize;
        int numDataSectors = sectorCount - reservedSectors - tableSectors - numRootSectors;
        int clusterCount = numDataSectors / bpb->SectorsPerCluster;
        int usedClusterCount = 0;
        int usedSectorCount = 0;

        buf = SafeAlloc(rootSize);
        ReadFile(buf, GetRootDir());

        const uint32_t *pClusterMap = GetClusterMap();
        for (int i = 0; i < clusterCount; i++)
        {
            uint32_t index = pClusterMap[i];
            if (IsClusterValid(index))
            {
                usedClusterCount++;
            }
        }

        usedSectorCount = (usedClusterCount * sectorsPerCluster)
            + numRootSectors + tableSectors + reservedSectors;

        int dataCapacity = clusterCount * clusterSize;
        int dataUsed = usedClusterCount * clusterSize;

        int diskCapacity = sectorCount * sectorSize;
        int diskUsed = usedSectorCount * sectorSize;

        float dataUsedPercent = ((float) dataUsed / dataCapacity) * 100.0f;
        float diskUsedPercent = ((float) diskUsed / diskCapacity) * 100.0f;

        printf("     Volume Label: %s\n", label);
        printf("    Serial Number: %04X-%04X\n",
            (bpb->VolumeId >> 16) & 0xFFFF, bpb->VolumeId & 0xFFFF);
        printf("  File System Tag: %s\n", fsId);
        printf("\n");
        printf("  Data Used/Total: %d/%d clusters (%.0f%%)\n",
            usedClusterCount, clusterCount, dataUsedPercent);
        printf("                   %d/%d bytes\n",
            dataUsed, dataCapacity);
        printf("\n");
        printf("Sectors per Track: %d\n", bpb->SectorsPerTrack);
        printf("            Heads: %d\n", bpb->HeadCount);
        printf("    Total Sectors: %d\n", bpb->SectorCount);
        printf("      Sector Size: %d\n", bpb->SectorSize);
        printf("     Cluster Size: %d\n", clusterSize);
        printf("    Media Type ID: 0x%02X\n", bpb->MediaType);
        printf("     Drive Number: %d\n", bpb->DriveNumber);
        printf("        FAT Count: %d\n", bpb->TableCount);
        printf("  Sectors per FAT: %d\n", bpb->SectorsPerTable);
        printf(" Reserved Sectors: %d\n", bpb->ReservedSectorCount);
        printf("   Hidden Sectors: %d\n", bpb->HiddenSectorCount);
        printf("    Large Sectors: %d\n", bpb->LargeSectorCount);
        printf("Root Dir Capacity: %d\n", bpb->MaxRootDirEntryCount);
        printf("Extended Boot Sig: 0x%02x\n", bpb->ExtendedBootSignature);
        printf("    Reserved Byte: 0x%02x\n", bpb->_Reserved);

        goto Cleanup;
    }
    else
    {
        //
        // File info.
        //

        DirEntry file;
        const char *path = args->Argv[1];
        success = FindFile(&file, path);
        if (!success)
        {
            LogError("file not found '%s'\n", path);
            goto Cleanup;
        }

        char shortname[MAX_SHORTNAME];
        GetShortName(shortname, &file);

        int size = GetFileSize(&file);
        int sizeOnDisk = GetFileSizeOnDisk(&file);
        int clusters = sizeOnDisk / GetClusterSize();

        char createdDate[MAX_DATE];
        char createdTime[MAX_TIME];
        char modifiedDate[MAX_DATE];
        char modifiedTime[MAX_TIME];
        char accessDate[MAX_DATE];

        GetDate(createdDate, &file.CreationDate);
        GetTimePrecise(createdTime, &file.CreationTime, file._Reserved2);
        GetDate(modifiedDate, &file.ModifiedDate);
        GetTime(modifiedTime, &file.ModifiedTime);
        GetDate(accessDate, &file.LastAccessDate);

        printf("        Name: %s\n", shortname);
        printf("        Size: %d %s\n", size, PLURALIZE("byte", size));
        printf("Size on disk: %d %s (%d %s)\n",
            sizeOnDisk, PLURALIZE("byte", sizeOnDisk),
            clusters, PLURALIZE("cluster", clusters));
        printf("     Created: %s %s\n", createdDate, createdTime);
        printf("    Modified: %s %s\n", modifiedDate, modifiedTime);
        printf("    Accessed: %s\n", accessDate);
        printf("  Attributes: 0x%02x", file.Attributes);
        if (file.Attributes != 0)
        {
            printf(" [");
            if (HasAttribute(&file, ATTR_READONLY))
                printf(" Read-Only");
            if (HasAttribute(&file, ATTR_HIDDEN))
                printf(" Hidden");
            if (HasAttribute(&file, ATTR_SYSTEM))
                printf(" System");
            if (HasAttribute(&file, ATTR_LABEL))
                printf(" Label");
            if (HasAttribute(&file, ATTR_DIRECTORY))
                printf(" Directory");
            if (HasAttribute(&file, ATTR_ARCHIVE))
                printf(" Archive");
            if (HasAttribute(&file, ATTR_DEVICE))
                printf(" Device");
            if (HasAttribute(&file, ATTR_LFN))
                printf(" Long File Name");
            printf(" ]\n");
        }
        printf("   Reserved: 0x%02x 0x%02x\n", file._Reserved1, file._Reserved3);
        printf("Cluster Map: ");

        uint32_t cluster = file.FirstCluster;
        int count = 0;
        while (IsClusterValid(cluster))
        {
            if ((count % 8) == 0)
            {
                printf("\n    ");
            }
            printf("%03x ", cluster);
            cluster = GetNextCluster(cluster);
            count++;
        }
        printf("\n");
    }

Cleanup:
    SafeFree(buf);
    CloseImage();
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}

int List(const Command *cmd, const CommandArgs *args)
{
    // TODO: --help
    // TODO: -r

    char *buf = NULL;
    bool success = true;
    bool listAll = false;
    bool bareFormat = false;
    bool shortNamesOnly = false;
    bool recurse = false;
    int c;

    DirEntry file;
    wchar_t name[MAX_PATH] = { 0 };
    char shortname[MAX_SHORTNAME] = { 0 };
    char month[MAX_DATE] = { 0 };
    char mode[2] = { 0 };

    opterr = 0;
    while ((c = getopt(args->Argc, args->Argv, "abnr")) != -1)
    {
        switch (c)
        {
            case 'a':
                listAll = true;
                break;
            case 'b':
                bareFormat = true;
                break;
            case 'n':
                shortNamesOnly = true;
                break;
            case 'r':
                recurse = true;
                break;
            case '?':
                GETOPT_INVALID();
                break;
        }
    }

    const char *path = args->Argv[optind];
    if (path == NULL)
    {
        path = "/";
    }

    RIF(OpenImage(args->ImagePath));

    success = FindFile(&file, path);
    if (!success)
    {
        LogError("file not found - %s\n", path);
        RIF(false);
    }

    int count = 1;
    const DirEntry *dirEntry = &file;

    if (IsDirectory(&file))
    {
        size_t size = GetFileSize(&file);

        buf = SafeAlloc(size);
        ReadFile(buf, &file);

        count = size / sizeof(DirEntry);
        dirEntry = (const DirEntry *) buf;

        // TODO: alphabetize
        // TODO: recurse
    }

    char lfnChecksum = 0;
    wchar_t lfn[MAX_PATH];
    bool hasLfn = false;

    int total = 0;
    for (int i = 0; i < count; i++)
    {
        const DirEntry *e = &dirEntry[i];
        if (IsLongFileName(e) && !IsDeleted(e))
        {
            hasLfn = ReadLongName(lfn, &lfnChecksum, &e);
            continue;
        }
        if (!IsFile(e))
        {
            hasLfn = false;
            continue;
        }
        if (!listAll)
        {
            if (IsParentDirectory(e) || IsCurrentDirectory(e) || IsHidden(e))
            {
                hasLfn = false;
                continue;
            }
        }

        total += GetFileSizeOnDisk(e) / GetClusterSize();

        if (IsDirectory(e))
        {
            mode[0] = 'd';
        }
        else if (IsDeviceFile(e))
        {
            mode[0] = 'i';
        }
        else
        {
            mode[0] = '-';
        }

        GetShortName(shortname, e);
        char checksum = GetShortNameChecksum(e);
        swprintf(name, MAX_PATH, L"%s", shortname);

        if (!shortNamesOnly &&hasLfn && checksum == lfnChecksum)
        {
            swprintf(name, MAX_PATH, L"%ls", lfn);
        }

        switch (e->ModifiedDate.Month)
        {
            case 1: sprintf(month, "Jan"); break;
            case 2: sprintf(month, "Feb"); break;
            case 3: sprintf(month, "Mar"); break;
            case 4: sprintf(month, "Apr"); break;
            case 5: sprintf(month, "May"); break;
            case 6: sprintf(month, "Jun"); break;
            case 7: sprintf(month, "Jul"); break;
            case 8: sprintf(month, "Aug"); break;
            case 9: sprintf(month, "Sep"); break;
            case 10:sprintf(month, "Oct"); break;
            case 11:sprintf(month, "Nov"); break;
            case 12:sprintf(month, "Dec"); break;
            default:sprintf(month, "   "); break;
        }

        if (bareFormat)
        {
            wprintf(L"%ls\n", name);
        }
        else
        {
            wprintf(L"%s %8d %s%3d%5d %02d:%02d %ls\n",
                mode,
                e->FileSize,
                month,
                e->ModifiedDate.Day,
                e->ModifiedDate.Year + YEAR_BASE,
                e->ModifiedTime.Hours,
                e->ModifiedTime.Minutes,
                name);
        }
    }

Cleanup:
    SafeFree(buf);
    CloseImage();
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}

int Mkdir(const Command *cmd, const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Move(const Command *cmd, const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Remove(const Command *cmd, const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Rename(const Command *cmd, const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Touch(const Command *cmd, const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Type(const Command *cmd, const CommandArgs *args)
{
    bool success = true;
    char shortname[MAX_SHORTNAME];
    char *buf = NULL;
    DirEntry file;

    if (args->Argc < 2)
    {
        LogError("missing file\n");
        return STATUS_INVALIDARG;
    }

    const char *path = args->Argv[1];
    RIF(OpenImage(args->ImagePath));

    success = FindFile(&file, path);
    if (!success)
    {
        LogError("file not found - %s\n", path);
        RIF(false);
    }

    size_t size = GetFileSize(&file);
    buf = SafeAlloc(size);
    ReadFile(buf, &file);

    if (IsDirectory(&file))
    {
        const DirEntry *e = (const DirEntry *) buf;
        int count = size / sizeof(DirEntry);

        for (int i = 0; i < count; i++, e++)
        {
            if (!IsFile(e))
            {
                // Skip free/deleted slots, LFNs, volume labels
                continue;
            }

            GetShortName(shortname, e);
            printf("%s\n", shortname);
        }
    }
    else
    {
        printf("%.*s", size, buf);
    }

Cleanup:
    SafeFree(buf);
    CloseImage();
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}
