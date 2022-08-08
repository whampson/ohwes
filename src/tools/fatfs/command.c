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
    { Add,
        "add", "add [OPTIONS] SOURCE[...]",
        "Add files to the disk image.",
        "SOURCE specifies the path to one or more files on the host system.\n"
        "Files are placed in the root directory unless -d is supplied.\n"
        "\n"
        "Options:\n"
        "    -d TARGETDIR    Add files to TARGETDIR.\n"
        "    -f              Overwrite files if they exist regardless of permissions.\n"
        "    -p              Create parent directories if they do not exist.\n"
    },
    { Attr,
        "attr", "attr [OPTIONS] FILE",
        "Change file attributes.",
        "Options:\n"
        "    -H              Set the 'hidden' bit.\n"
        "    -R              Set the 'read-only' bit.\n"
        "    -S              Set the 'system file' bit.\n"
        "    -h              Clear the 'hidden' bit.\n"
        "    -r              Clear the 'read-only' bit.\n"
        "    -s              Clear the 'system file' bit.\n"
        "    -X VALUE        Set the attributes byte to VALUE.\n"
    },
    { Copy,
        "copy", "copy [OPTIONS] SOURCE TARGET",
        "Copy the contents of a file or directory.",
        "Options:\n"
        "    -f              Overwrite TARGET if it exists regardless of permissions.\n"
        "    -p              Create parent directories if they do not exist.\n"
        "    -r              Copy subdirectories.\n"
    },
    { Create,
        "create", "create [OPTIONS]",
        "Create a new FAT-formatted disk image.",
        "DISKIMAGE will be used as the path to create the new disk image.\n"
        "\n"
        "Options:\n"
        "    -L              Set the volume label.\n"
        "    --force         Overwrite the disk image file if it exists.\n",
    },
    { Extract,
        "extract", "extract [OPTIONS] SOURCE[...]",
        "Extract files from the disk image.",
        "SOURCE specifies the path to one or more files on the disk image.\n"
        "Files are placed in the current working directory unless -d is supplied.\n"
        "\n"
        "Options:\n"
        "    -d TARGETDIR    Extract files to TARGETDIR.\n"
        "    -f              Overwrite files if they exist.\n"
        "    -r              Extract subdirectories.\n"
    },
    { Help,
        "help", "help COMMAND",
        "Print help about a command.",
        NULL,
    },
    // { Info,
    //     "info", "info [FILE]",
    //     "Print file, directory, or disk information.",
    //     NULL,
    // },
    { List,
        "list", "list [OPTIONS] [FILE]",
        "List information about a file (the root directory by default).",
        "Options:\n"
        "  -a       List all files; include hidden files.\n"
        "  -b       Bare format; print file names only.\n"
        "  -n       Use short names only.\n"
        "  -r       Print the contents of subdirectories.\n"
    },
    { Mkdir,
        "mkdir", "mkdir [OPTIONS] PATH",
        "Create a directory.",
        "Options:\n"
        "    -p              Create parent directories if they do not exist.\n"
    },
    { Move,
        "move", "move [OPTIONS] SOURCE TARGET",
        "Move a file or directory to a new location.",
        "Options:\n"
        "    -f              Overwrite TARGET if it exists regardless of permissions.\n"
        "    -p              Create parent directories if they do not exist.\n"
    },
    { Remove,
        "remove", "remove [OPTIONS] FILE",
        "Remove a file or directory.",
        "Options:\n"
        "    -f              Remove PATH regardless of permissions or directory\n"
        "                    contents.\n"
    },
    { Rename,
        "rename", "rename FILE NEWNAME",
        "Rename a file or directory.",
        NULL,
    },
    { Touch,
        "touch", "touch [OPTIONS] FILE",
        "Change file access and modification times.",
        "By default, both the access and modification times are updated to the current\n"
        "date and time. If FILE does not exist, it will be created as an empty file.\n"
        "\n"
        "Options:\n"
        "    -a              Change the access time. Modification time is not updated\n"
        "                    unless -m is specified.\n"
        "    -m              Change the modification time. Access time is not updated\n"
        "                    unless -a is specified.\n"
        "    -d MMDDYYYY     Set access and/or modification date to MMDDYYYY.\n"
        "                        MM      Month, from 01 to 12.\n"
        "                        DD      Day, from 01 to 31.\n"
        "                        YYYY    Year, from 1980 to 2107.\n"
        "    -t hh:mm:ss     Set access and/or modification time to hh:mm:ss.\n"
        "                        hh      Hours, from 00 to 23.\n"
        "                        mm      Minutes, from 00 to 59.\n"
        "                        ss      Seconds, from 00 to 59.\n"
        "    -f              Update access and modification times regardless of\n"
        "                    permissions.\n"
    },
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

int Add(const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Attr(const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Create(const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Copy(const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Extract(const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Help(const CommandArgs *args)
{
    if (args->Argc < 2)
    {
        PrintUsage();
        return STATUS_SUCCESS;
    }

    const char *cmdName = args->Argv[1];
    const Command *cmd = FindCommand(cmdName);
    if (cmd == NULL)
    {
        LogError("help: invalid command - %s\n", cmdName);
        return STATUS_INVALIDARG;
    }

    if (cmd->Usage)
    {
        printf("Usage: %s\n", cmd->Usage);
    }
    if (cmd->ShortHelp)
    {
        printf("%s\n", cmd->ShortHelp);
    }
    if (cmd->LongHelp)
    {
        printf("\n%s", cmd->LongHelp);
    }

    return STATUS_SUCCESS;
}

int Info(const CommandArgs *args)
{
    bool success = true;

//     RIF(OpenImage(args->ImagePath));

//     if (args->Argc == 0)
//     {
//         const BiosParamBlock *bpb = GetBiosParams();

//         char label[LABEL_LENGTH + 1];
//         char fsId[NAME_LENGTH + 1];

//         GetLabel(label, bpb->Label);
//         GetName(fsId, bpb->FileSystemType);

//         int sectorSize = bpb->SectorSize;
//         int sectorCount = bpb->SectorCount;
//         int sectorsPerCluster = bpb->SectorsPerCluster;
//         int clusterSize = sectorSize * sectorsPerCluster;
//         int clusterCount = sectorCount / sectorsPerCluster;
//         int diskSize = clusterCount * clusterSize;
//         int tableSectors = bpb->TableCount * bpb->SectorsPerTable;
//         int rootDirSectors = bpb->MaxRootDirEntryCount * sizeof(DirEntry) / sectorSize;

//         // TODO: extended boot signature?

//         printf("%s info:\n", GetImagePath());
//         printf("The disk has %d heads, %d sectors per track, and a sector size of %d bytes.\n",
//             bpb->HeadCount, bpb->SectorsPerTrack, sectorSize);
//         printf("The media type ID is %02X; there are %d sectors for a total disk size of %d bytes.\n",
//             bpb->MediaType, sectorCount, diskSize);
//         printf("There %s %d hidden %s, %d large %s, and %d reserved %s.\n",
//             ISARE(bpb->HiddenSectorCount), bpb->HiddenSectorCount, PLURAL("sector", bpb->HiddenSectorCount),
//             bpb->LargeSectorCount, PLURAL("sector", bpb->LargeSectorCount),
//             bpb->ReservedSectorCount, PLURAL("sector", bpb->ReservedSectorCount));
//         printf("The cluster size is %d bytes.\n", clusterSize);
//         printf("The drive number is %d.\n", bpb->DriveNumber);
//         printf("There %s %d %s occupying %d sectors.\n",
//             ISARE(bpb->TableCount), bpb->TableCount, PLURAL("FAT", bpb->TableCount),
//             tableSectors);
//         printf("The root directory contains %d slots and occupies %d sectors.\n",
//             bpb->MaxRootDirEntryCount, rootDirSectors);
//         switch (bpb->ExtendedBootSignature)
//         {
//             case 0x28:
//                 printf("The volume ID is %08X.\n", bpb->VolumeId);
//                 break;
//             case 0x29:
//                 printf("The volume ID is %08X; the volume label is '%s'.\n",
//                     bpb->VolumeId, label);
//                 printf("The file system type is '%s'.\n", fsId);
//                 break;
//             default:
//                 printf(".\n");
//                 break;

//         }
//         printf("The reserved byte is 0x%02x.\n", bpb->_Reserved);

//         // TODO: bytes used/free, file count, dir count
//         return STATUS_SUCCESS;
//     }

//     const char *path = args->Argv[0];
//     DirEntry e;
//     DirEntry *file = &e;
//     success = FindFile(file, path);
//     if (!success)
//     {
//         LogError("file not found - %s\n", path);
//         goto Cleanup;
//     }

//     char shortName[MAX_SHORTNAME] = { 0 };
//     char dateBuf[MAX_DATE];
//     char timeBuf[MAX_TIME];

//     bool isDir = IsDirectory(file);
//     bool isRoot = IsRoot(file);

//     if (isRoot)
//     {
//         printf("%s is the root directory.\n", path);
//         goto Cleanup;
//     }

//     int clusterCount = 0;
//     uint32_t cluster = file->FirstCluster;
//     while (IsClusterValid(cluster))
//     {
//         clusterCount++;
//         cluster = GetNextCluster(cluster);
//     }

//     printf("%s is a ", path);
//     if (IsReadOnly(file))
//     {
//         printf("read-only ");
//     }
//     if (IsHidden(file))
//     {
//         printf("hidden ");
//     }
//     if (IsSystemFile(file))
//     {
//         printf("system ");
//     }
//     if (IsDirectory(file))
//     {
//         printf("directory ");
//     }
//     else
//     {
//         printf("file ");
//     }
//     printf("occupying %d %s:\n",
//         clusterCount, PLURAL("cluster", clusterCount));

//     cluster = file->FirstCluster;
//     clusterCount = 0;

//     while (IsClusterValid(cluster))
//     {
//         printf("  %03x", cluster);
//         clusterCount++;
//         if (clusterCount % 8 == 0)
//         {
//             printf("\n");
//         }
//         cluster = GetNextCluster(cluster);
//     }
//     printf("\n");

//     GetShortName(shortName, file);
//     printf("The %s short name is '%s'.\n",
//         (isDir) ? "directory" : "file",
//         shortName);

//     printf("The %s size is %d bytes.\n",
//         (isDir) ? "directory" : "file",
//         GetFileSize(file));
//     printf("The %s size on disk is %d bytes.\n",
//         (isDir) ? "directory" : "file",
//         GetFileSizeOnDisk(file));

//     GetDate(dateBuf, &file->CreationDate);
//     GetTime(timeBuf, &file->CreationTime);
//     printf("The %s was created %s %s.\n",
//         (isDir) ? "directory" : "file",
//         dateBuf, timeBuf);

//     GetDate(dateBuf, &file->ModifiedDate);
//     GetTime(timeBuf, &file->ModifiedTime);
//     printf("The %s was modified %s %s.\n",
//         (isDir) ? "directory" : "file",
//         dateBuf, timeBuf);

//     GetDate(dateBuf, &file->LastAccessDate);
//     printf("The %s was last accessed on %s.\n",
//         (isDir) ? "directory" : "file",
//         dateBuf);

//     printf("The attribute byte is 0x%02X; ", file->Attributes);
//     printf("the reserved bytes are 0x%02X, 0x%02X, and 0x%02X.\n",
//         file->_Reserved1, file->_Reserved2, file->_Reserved3);

// Cleanup:
//     CloseImage();
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}

int List(const CommandArgs *args)
{
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

    size_t size = GetFileSize(&file);
    char *buf = SafeAlloc(size);
    ReadFile(buf, &file);

    int count = 1;
    const DirEntry *dirEntry = &file;
    if (IsDirectory(&file))
    {
        count = size / sizeof(DirEntry);
        dirEntry = (const DirEntry *) buf;

        // TODO: alphabetize
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

int Mkdir(const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Move(const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Remove(const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Rename(const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Touch(const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Type(const CommandArgs *args)
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
