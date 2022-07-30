#include "command.h"
#include "image.h"

int Type(const CommandArgs *args)
{
    char *data = NULL;
    bool success = true;

    if (args->Argc == 0)
    {
        LogError("missing file\n");
        return STATUS_INVALIDARG;
    }

    success = OpenImage(args->ImagePath);
    if (!success)
    {
        goto Cleanup;
    }

    const char *path = args->Argv[0];
    const DirEntry * dirEntry = FindFile(path);
    if (!dirEntry)
    {
        LogError("file not found - %s\n", path);
        success = false;
        goto Cleanup;
    }

    if (dirEntry->Attributes & ATTR_DIRECTORY)
    {
        // This is hairy...
        // TODO: somehow get the root dir to work

        const int ClusterSize = GetClusterSize();
        const int NumDirEntriesPerCluster = ClusterSize / sizeof(DirEntry);

        char buf[ClusterSize];
        char entryName[MAX_SHORTNAME] = { 0 };
        uint32_t cluster = dirEntry->FirstCluster;

        while (CLUSTER_IS_VALID(cluster))
        {
            ReadCluster(cluster, buf);
            cluster = GetNextCluster(cluster);

            DirEntry *entry = (DirEntry *) buf;
            for (int i = 0; i < NumDirEntriesPerCluster; i++, entry++)
            {
                // TODO: validate dir entry somehow
                switch ((unsigned char) entry->Name[0])
                {
                    case 0x00:
                    case 0x05:
                    case 0xE5:
                        // free slot/deleted file
                        continue;
                }

                if (IsFlagSet(entry->Attributes, ATTR_LFN))
                {
                    // Skip LFN entries
                    continue;
                }

                GetShortName(entryName, entry);
                printf("%s\n", entryName);
            }
        }
    }
    else
    {
        int size = dirEntry->FileSize;
        data = SafeAlloc(size);

        success = ReadFile(dirEntry, data);
        if (success)
        {
            printf("%.*s", size, data);
        }
    }

Cleanup:
    CloseImage();
    SafeFree(data);
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}
