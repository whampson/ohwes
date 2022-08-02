#include "command.h"
#include "image.h"

int Info(const CommandArgs *args)
{
    bool success = true;

    success = OpenImage(args->ImagePath);
    if (!success)
    {
        goto Cleanup;
    }

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
    DirEntry *e = NULL;
    success = FindFile(&e, path);
    if (!success)
    {
        LogError("info: file not found - %s\n", path);
        return STATUS_ERROR;
    }

    char shortName[MAX_SHORTNAME] = { 0 };
    char dateBuf[MAX_DATE];
    char timeBuf[MAX_TIME];

    GetShortName(shortName, e);
    printf("%s info:\n", shortName);

    int clusterCount = 0;
    uint32_t cluster = e->FirstCluster;
    while (cluster >= CLUSTER_FIRST && cluster <= CLUSTER_LAST)
    {
        clusterCount++;
        cluster = GetNextCluster(cluster);
    }

    bool isDirectory = false;

    printf("%s is a ", shortName);
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
    while (cluster >= CLUSTER_FIRST && cluster <= CLUSTER_LAST)
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

    printf("The short name is '%s'.\n", shortName);

    if (!isDirectory)
    {
        printf("The file size is %d bytes.\n",
            e->FileSize);
    }
    printf("The %s size on disk is %d bytes.\n",
        (isDirectory) ? "directory" : "file",
        GetFileSizeOnDisk(e));

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

Cleanup:
    CloseImage();
    SafeFree(e);
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}
