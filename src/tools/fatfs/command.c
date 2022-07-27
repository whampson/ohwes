#include <stdarg.h>
#include "command.h"
#include "image.h"

static int Help(int argc, const char **argv);
static int Info(int argc, const char **argv);
static int Type(int argc, const char **argv);

static const Command s_pCommands[] =
{
    { "help",
        "help [COMMAND]",
        "Print help information about this program or COMMAND.",
        NULL,
        Help
    },
    { "info",
        "info [PATH]",
        "Print information about PATH or the disk image itself.",
        NULL,
        Info
    },
    { "type",
        "type PATH",
        "Print the contents PATH.",
        NULL,
        Type
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

static int Help(int argc, const char **argv)
{
    if (argc == 0)
    {
        Usage();
        return STATUS_SUCCESS;
    }

    const char *cmdName = argv[0];
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
    if (cmd->Desc)
    {
        printf("%s\n", cmd->Desc);
    }
    if (cmd->Help)
    {
        printf("\n%s\n", cmd->Help);
    }

    return STATUS_SUCCESS;
}

static int Info(int argc, const char **argv)
{
    if (argc == 0)
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
        printf("There root contains %d slots and occupies %d sectors.\n",
            bpb->MaxRootDirEntryCount, rootDirSectors);
        switch (bpb->ExtendedBootSignature)
        {
            case 0x28:
                printf("The volume ID is %08X.\n", bpb->VolumeId);
                break;
            case 0x29:
                printf("The volume ID is %08X; the volume label is '%s'\n",
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

    const char *path = argv[0];
    const DirectoryEntry * dirEntry = FindFile(path);
    if (!dirEntry)
    {
        LogError("info: file not found - %s\n", path);
        return STATUS_ERROR;
    }

    char name[NAME_LENGTH + 1] = { 0 };
    char ext[EXTENSION_LENGTH + 1] = { 0 };
    GetName(name, dirEntry->Name);
    GetExt(ext, dirEntry->Extension);

    printf("%s", name);
    if (ext[0] != '\0')
    {
        printf(".%s", ext);
    }
    printf(" info:\n");

    int clusterCount = 0 ;
    uint32_t cluster = dirEntry->FirstCluster;
    while (cluster != CLUSTER_END)
    {
        clusterCount++;
        cluster = GetNextCluster(cluster);
    }

    printf("The file is %d bytes in size, occupying %d %s.\n",
        dirEntry->FileSize, clusterCount, PLURAL("cluster", clusterCount));

    return STATUS_SUCCESS;
}

static int Type(int argc, const char **argv)
{
    if (argc == 0)
    {
        LogError("type: missing file\n");
        return STATUS_INVALIDARG;
    }

    const char *path = argv[0];
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
