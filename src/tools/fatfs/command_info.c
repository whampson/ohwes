#include "command.h"
#include "image.h"

int Info(const CommandArgs *args)
{
    bool success = true;

    RIF(OpenImage(args->ImagePath));

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
        int rootDirSectors = bpb->MaxRootDirEntryCount * sizeof(DirEntry) / sectorSize;

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
    DirEntry e;
    DirEntry *file = &e;
    success = FindFile(file, path);
    if (!success)
    {
        LogError("file not found - %s\n", path);
        goto Cleanup;
    }

    char shortName[MAX_SHORTNAME] = { 0 };
    char dateBuf[MAX_DATE];
    char timeBuf[MAX_TIME];

    bool isDir = IsDirectory(file);
    bool isRoot = IsRoot(file);

    if (isRoot)
    {
        printf("%s is the root directory.\n", path);
        goto Cleanup;
    }

    int clusterCount = 0;
    uint32_t cluster = file->FirstCluster;
    while (IsClusterValid(cluster))
    {
        clusterCount++;
        cluster = GetNextCluster(cluster);
    }

    printf("%s is a ", path);
    if (IsReadOnly(file))
    {
        printf("read-only ");
    }
    if (IsHidden(file))
    {
        printf("hidden ");
    }
    if (IsSystemFile(file))
    {
        printf("system ");
    }
    if (IsDirectory(file))
    {
        printf("directory ");
    }
    else
    {
        printf("file ");
    }
    printf("occupying %d %s:\n",
        clusterCount, PLURAL("cluster", clusterCount));

    cluster = file->FirstCluster;
    clusterCount = 0;

    while (IsClusterValid(cluster))
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

    GetShortName(shortName, file);
    printf("The %s short name is '%s'.\n",
        (isDir) ? "directory" : "file",
        shortName);

    printf("The %s size is %d bytes.\n",
        (isDir) ? "directory" : "file",
        GetFileSize(file));
    printf("The %s size on disk is %d bytes.\n",
        (isDir) ? "directory" : "file",
        GetFileSizeOnDisk(file));

    GetDate(dateBuf, &file->CreationDate);
    GetTime(timeBuf, &file->CreationTime);
    printf("The %s was created %s %s.\n",
        (isDir) ? "directory" : "file",
        dateBuf, timeBuf);

    GetDate(dateBuf, &file->ModifiedDate);
    GetTime(timeBuf, &file->ModifiedTime);
    printf("The %s was modified %s %s.\n",
        (isDir) ? "directory" : "file",
        dateBuf, timeBuf);

    GetDate(dateBuf, &file->LastAccessDate);
    printf("The %s was last accessed on %s.\n",
        (isDir) ? "directory" : "file",
        dateBuf);

    printf("The attribute byte is 0x%02X; ", file->Attributes);
    printf("the reserved bytes are 0x%02X, 0x%02X, and 0x%02X.\n",
        file->_Reserved1, file->_Reserved2, file->_Reserved3);

Cleanup:
    CloseImage();
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}
