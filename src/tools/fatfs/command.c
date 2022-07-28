#include <stdarg.h>
#include "command.h"
#include "image.h"

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
    { Info,
        "info", "info [FILE]",
        "Print file, directory, or disk information.",
        NULL,
    },
    { List,
        "list", "list [OPTIONS] FILE",
        "Print the contents of a directory.",
        "Options:\n"
        "    -a              List all files, including hidden files, volume labels,\n"
        "                    and device files.\n"
        "    -b              Bare format; print names only.\n"
        "    -r              Print the contents of subdirectories.\n"
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
    if (args->Argc == 0)
    {
        Usage();
        return STATUS_SUCCESS;
    }

    const char *cmdName = args->Argv[0];
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
    if (args->Argc == 0)
    {
        const BiosParamBlock *bpb = GetBiosParams();

        char label[LABEL_LENGTH + 1];
        char fsId[NAME_LENGTH + 1];

        GetLabel(label, bpb->Label);
        GetName(fsId, bpb->FileSystemType);

        int sectorSize = bpb->SectorSize;
        int sectorCount = bpb->SectorCount;
        int sectorsPerCluster = bpb->SectorsPerCluster;
        int clusterSize = sectorSize * sectorsPerCluster;
        int clusterCount = sectorCount / sectorsPerCluster;
        int diskSize = clusterCount * clusterSize;
        int tableSectors = bpb->TableCount * bpb->SectorsPerTable;
        int rootDirSectors = bpb->MaxRootDirEntryCount * sizeof(DirectoryEntry) / sectorSize;

        // TODO: extended boot signature?

        printf("%s info:\n", GetImagePath());
        printf("The disk has %d heads, %d sectors per track, and a sector size of %d bytes.\n",
            bpb->HeadCount, bpb->SectorsPerTrack, sectorSize);
        printf("The media type ID is %02X; there are %d sectors for a total disk size of %d bytes.\n",
            bpb->MediaType, sectorCount, diskSize);
        printf("There %s %d hidden %s, %d large %s, and %d reserved %s.\n",
            ISARE(bpb->HiddenSectorCount), bpb->HiddenSectorCount, PLURAL("sector", bpb->HiddenSectorCount),
            bpb->LargeSectorCount, PLURAL("sector", bpb->LargeSectorCount),
            bpb->ReservedSectorCount, PLURAL("sector", bpb->ReservedSectorCount));
        printf("The cluster size is %d bytes.\n", clusterSize);
        printf("The drive number is %d.\n", bpb->DriveNumber);
        printf("There %s %d %s occupying %d sectors.\n",
            ISARE(bpb->TableCount), bpb->TableCount, PLURAL("FAT", bpb->TableCount),
            tableSectors);
        printf("The root directory contains %d slots and occupies %d sectors.\n",
            bpb->MaxRootDirEntryCount, rootDirSectors);
        switch (bpb->ExtendedBootSignature)
        {
            case 0x28:
                printf("The volume ID is %08X.\n", bpb->VolumeId);
                break;
            case 0x29:
                printf("The volume ID is %08X; the volume label is '%s'.\n",
                    bpb->VolumeId, label);
                printf("The file system type is '%s'.\n", fsId);
                break;
            default:
                printf(".\n");
                break;

        }
        printf("The reserved byte is 0x%02x.\n", bpb->_Reserved);

        // TODO: bytes used/free, file count, dir count
        return STATUS_SUCCESS;
    }

    const char *path = args->Argv[0];
    const DirectoryEntry * e = FindFile(path);
    if (!e)
    {
        LogError("info: file not found - %s\n", path);
        return STATUS_ERROR;
    }

    char dateBuf[DATE_LENGTH];
    char timeBuf[TIME_LENGTH];
    char nameBuf[NAME_LENGTH + 1] = { 0 };
    char extBuf[EXTENSION_LENGTH + 1] = { 0 };
    char fullName[NAME_LENGTH + EXTENSION_LENGTH + 2] = { 0 };

    GetName(nameBuf, e->Name);
    GetExt(extBuf, e->Extension);
    size_t nameLen = strlen(nameBuf);
    size_t extLen = strlen(extBuf);

    snprintf(fullName, nameLen + 1, "%s", nameBuf);
    if (extLen != 0)
    {
        snprintf(fullName + nameLen, extLen + 2, ".%s", extBuf);
    }
    printf("%s info:\n", fullName);

    int clusterCount = 0;
    uint32_t cluster = e->FirstCluster;
    while (cluster != CLUSTER_END)
    {
        clusterCount++;
        cluster = GetNextCluster(cluster);
    }

    bool isDirectory = false;

    printf("%s is a ", fullName);
    if (e->Attributes & ATTR_READONLY)
    {
        printf("read-only ");
    }
    if (e->Attributes & ATTR_HIDDEN)
    {
        printf("hidden ");
    }
    // if (e->Attributes & ATTR_ARCHIVE)
    // {
    //     printf("archived ");
    // }
    if (e->Attributes & ATTR_LABEL)
    {
        printf("volume label ");
    }
    if (e->Attributes & ATTR_SYSTEM)
    {
        printf("system file ");
    }
    if (e->Attributes & ATTR_DEVICE)
    {
        printf("device file ");
    }
    if (e->Attributes & ATTR_DIRECTORY)
    {
        isDirectory = true;
        printf("directory ");
    }
    else
    {
        printf("file ");
    }
    printf("occupying %d %s:\n", clusterCount, PLURAL("cluster", clusterCount));

    cluster = e->FirstCluster;
    clusterCount = 0;
    while (cluster != CLUSTER_END)
    {
        printf("  %03x", cluster);
        clusterCount++;
        if (clusterCount % 8 == 0)
        {
            printf("\n");
        }
        cluster = GetNextCluster(cluster);
    }
    printf("\n");

    if (!isDirectory)
    {
        printf("The file is %d bytes in size.\n",
            e->FileSize);
    }

    GetDate(dateBuf, &e->CreationDate);
    GetTime(timeBuf, &e->CreationTime);
    printf("The %s was created %s %s.\n",
        (isDirectory) ? "directory" : "file",
        dateBuf, timeBuf);

    GetDate(dateBuf, &e->ModifiedDate);
    GetTime(timeBuf, &e->ModifiedTime);
    printf("The %s was modified %s %s.\n",
        (isDirectory) ? "directory" : "file",
        dateBuf, timeBuf);

    GetDate(dateBuf, &e->LastAccessDate);
    printf("The %s was last accessed on %s.\n",
        (isDirectory) ? "directory" : "file",
        dateBuf);

    printf("The attributes byte is 0x%02X; ", e->Attributes);
    printf("the reserved bytes are 0x%02X, 0x%02X, and 0x%02X.\n",
        e->_Reserved1, e->_Reserved2, e->_Reserved3);

    return STATUS_SUCCESS;
}

int List(const CommandArgs *args)
{
    return STATUS_ERROR;
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
    if (args->Argc == 0)
    {
        LogError("type: missing file\n");
        return STATUS_INVALIDARG;
    }

    const char *path = args->Argv[0];
    const DirectoryEntry * dirEntry = FindFile(path);
    if (!dirEntry)
    {
        LogError("type: file not found - %s\n", path);
        return STATUS_ERROR;
    }

    bool success = true;
    char *data;
    int size = dirEntry->FileSize;
    SafeAlloc(data, size);

    success = ReadFile(dirEntry, data);
    if (success)
    {
        printf("%.*s", size, data);
    }

Cleanup:
    SafeFree(data);
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}
