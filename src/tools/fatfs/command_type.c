#include "command.h"
#include "image.h"

int Type(const CommandArgs *args)
{
    bool success = true;
    char shortname[MAX_SHORTNAME];
    char *buf = NULL;
    const char *path = args->Argv[0];
    DirEntry file;

    if (args->Argc == 0)
    {
        LogError("missing file\n");
        return STATUS_INVALIDARG;
    }

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
