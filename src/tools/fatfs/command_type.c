#include "command.h"
#include "image.h"

int Type(const CommandArgs *args)
{
    char *data = NULL;
    const DirEntry *e = NULL;
    DirEntry *dirEntry = NULL;
    bool success = true;
    char shortnameBuf[MAX_SHORTNAME];

    if (args->Argc == 0)
    {
        LogError("missing file\n");
        return STATUS_INVALIDARG;
    }

    RIF(OpenImage(args->ImagePath));

    const char *path = args->Argv[0];
    success = FindFile(&dirEntry, path);
    if (!success)
    {
        LogError("file not found - %s\n", path);
        success = false;
        goto Cleanup;
    }

    bool isRoot = (dirEntry == NULL);
    size_t size = GetFileSizeOnDisk(dirEntry);

    if (!isRoot)
    {
        data = SafeAlloc(size);
        RIF(ReadFile(data, dirEntry));

        if (!IsDirectory(dirEntry))
        {
            printf("%.*s", dirEntry->FileSize, data);
            goto Cleanup;
        }

        e = (const DirEntry *) data;
    }
    else
    {
        e = GetRootDir();
    }

    int count = size / sizeof(DirEntry);
    for (int i = 0; i < count; i++, e++)
    {
        // TODO: validate dir entry somehow
        switch ((unsigned char) e->Name[0])
        {
            case 0x00:
            case 0x05:
            case 0xE5:
                // free slot/deleted file
                continue;
        }

        if (IsFlagSet(e->Attributes, ATTR_LFN))
        {
            // Skip LFN entries
            continue;
        }

        GetShortName(shortnameBuf, e);
        printf("%s\n", shortnameBuf);
    }

Cleanup:
    SafeFree(data);
    SafeFree(dirEntry);
    CloseImage();
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}
