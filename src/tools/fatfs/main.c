#include "diskimage.h"

#define PROG_NAME   "fatfs"

void Usage(void)
{
    printf("Usage: %s <image_file> <command> [args]\n", PROG_NAME);
    printf("\n");
    printf("Available Commands:\n");
    printf("  info          print disk info\n");
    printf("  type          print the contents of a file\n");

}

int ProcessCommand(int argc, const char **argv)
{
    const char *cmd = argv[2];
    const char **args = &argv[3];

    if (strcmp(cmd, "info") == 0)
    {
        PrintDiskInfo();
    }
    else if (strcmp(cmd, "type") == 0)
    {
        if (argc < 4)
        {
            LogError("missing file\n");
            return 1;
        }

        const DirectoryEntry *entry = FindFile(args[0]);
        if (entry == NULL)
        {
            LogError("file not found\n");
            return 2;
        }

        char *file = malloc(entry->FileSize + 1);
        if (!file)
        {
            LogError("out of memory!\n");
            return 2;
        }

        memset(file, 0, entry->FileSize + 1);
        if (!ReadFile(entry, file))
        {
            return 2;
        }

        printf("%s\n", file);
        free(file);
    }
    else
    {
        return 1;
    }

    return 0;
}

int main(int argc, const char **argv)
{
    if (argc < 3)
    {
        Usage();
        return 1;
    }

    if (!OpenImage(argv[1]))
    {
        return 2;
    }

    int retVal = ProcessCommand(argc, argv);
    CloseImage();

    return retVal;
}
