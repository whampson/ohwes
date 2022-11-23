#include "Command.hpp"
#include "DiskImage.hpp"

static const Command s_pCommands[] =
{
    { Create,   // similar to mkdosfs
        "create", "create [OPTIONS] DISKIMAGE",
        "Create a new FAT-formatted disk image.",
        "  -c COUNT      Create a disk image with COUNT sectors\n"
        "  -d NUMBER     Set the drive number to NUMBER\n"
        "  -f COUNT      Create COUNT file allocation tables\n"
        "  -F WIDTH      Select the FAT width (12, or 16)\n"
        "  -g HEADS/SPT  Select the disk geometry (as heads/sectors_per_track)\n"
        "  -h COUNT      Set the number of hidden sectors to COUNT\n"
        "  -i VOLID      Set the volume ID to VOLID (as a 32-bit hex number)\n"
        "  -l LABEL      Set the volume label to LABEL (11 chars max)\n"
        "  -m TYPE       Set the media type ID to TYPE\n"
        "  -r COUNT      Create space for COUNT root directory entries\n"
        "  -R COUNT      Create COUNT reserved sectors\n"
        "  -s COUNT      Set the number of sectors per cluster to COUNT\n"
        "  -S SIZE       Set the sector size to SIZE (power of 2, minimum 512)\n"
        "  --force       Overwrite the disk image file if it already exists\n"
        "  --help        Show this help text\n",
    },
    { Help,
        "help", "help [COMMAND]",
        "Get help about a command, or generic help about " PROG_NAME ".",
        NULL
    }
};

const Command * GetCommands()
{
    return s_pCommands;
}

int GetCommandCount()
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

void PrintCommandHelp(const Command *cmd)
{
    if (cmd->Synopsis)
    {
        printf("Usage: %s\n", cmd->Synopsis);
    }
    if (cmd->Descripton)
    {
        printf("%s\n", cmd->Descripton);
    }
    if (cmd->Options)
    {
        printf("\nOptions:\n%s\n", cmd->Options);
    }
}

int Create(const Command *cmd, const CommandArgs *args)
{
    const char *path = NULL;

    // Option variales, defaults for 1440k (3.5in) floppy disk
    int sectorSize = 512;
    int sectorCount = 2880;
    int headCount = 2;
    int sectorsPerTrack = 18;
    int sectorsPerCluster = 1;
    int mediaType = MEDIATYPE_1440K;
    int driveNumber = 0;
    int fatCount = 2;
    int fatWidth = 12;        // TODO: 16, 32?
    int rootCapacity = 224;
    int reservedCount = 1;
    int hiddenCount = 0;
    int volumeId = time(NULL);
    const char *label = "NO NAME";

    // Option booleans
    int bHelp = 0;
    int bForce = 0;

    static struct option LongOptions[] =
    {
        { "help", no_argument, &bHelp, 1 },
        { "force", no_argument, &bForce, 1 },
        { 0, 0, 0, 0 }
    };

    optind = 0;     // reset option index
    opterr = 0;     // prevent default error messages

    // Parse arguments
    bool success = true;
    while (true)
    {
        int optIdx = 0;
        int c = getopt_long(
            args->Argc, args->Argv,
            "+:c:d:f:F:g:h:i:l:m:r:R:s:S:",
            LongOptions, &optIdx);

        if (c == -1 || !success)
            break;

        if (bHelp)
        {
            PrintCommandHelp(cmd);
            return STATUS_SUCCESS;
        }

        switch (c)
        {
            case 0:
                if (LongOptions[optIdx].flag != 0)
                    break;
                assert(!"unhandled getopt_long case!");
                break;
            case 'c':
                sectorCount = strtol(optarg, NULL, 0);
                break;
            case 'd':
                driveNumber = strtol(optarg, NULL, 0);
                break;
            case 'f':
                fatCount = strtol(optarg, NULL, 0);
                break;
            case 'F':
                fatWidth = strtol(optarg, NULL, 0);
                break;
        case 'g':
                // TODO: better error handling
                optarg = strtok(optarg, "/");
                if (!optarg)
                {
                    LogError("invalid geometry format\n");
                    success = false;
                    break;
                }
                headCount = strtol(optarg, NULL, 0);

                optarg = strtok(NULL, "/");
                if (!optarg)
                {
                    LogError("invalid geometry format\n");
                    success = false;
                    break;
                }
                sectorsPerTrack = strtol(optarg, NULL, 0);
                break;
            case 'h':
                hiddenCount = strtol(optarg, NULL, 0);
                break;
            case 'i':
                volumeId = strtol(optarg, NULL, 16);
                break;
            case 'l':
                label = optarg;
                break;
            case 'm':
                mediaType = strtol(optarg, NULL, 0);
                break;
            case 'r':
                rootCapacity = strtol(optarg, NULL, 0);
                break;
            case 'R':
                reservedCount = strtol(optarg, NULL, 0);
                break;
            case 's':
                sectorsPerCluster = strtol(optarg, NULL, 0);
                break;
            case 'S':
                sectorSize = strtol(optarg, NULL, 0);
                break;
            case '?':
                if (optopt != 0)
                    BAD_OPT(optopt);
                else
                    BAD_LONGOPT(&args->Argv[optind - 1][2]);
                success = false;
                break;
            case ':':
                if (optopt != 0)
                    MISSING_OPTARG(optopt);
                else
                    MISSING_LONGOPTARG(&args->Argv[optind - 1][2]);
                success = false;
                break;
            default:
                assert(!"unhandled getopt_long case!");
                break;
        }
    }

    if (!success)
    {
        return STATUS_INVALIDARG;
    }

    // TODO: validate params to some extent

    // Read the path
    if (optind < args->Argc)
    {
        path = args->Argv[optind];
    }
    if (!path)
    {
        LogError("missing disk image file name\n");
        return STATUS_INVALIDARG;
    }

    // Test whether the file exists,
    // fail if it does and --force not specified
    FILE *fp = fopen(path, "r");
    if (fp != NULL)
    {
        if (!bForce)
        {
            LogError("%s exists\n", path);
            fclose(fp);
            return STATUS_ERROR;
        }
        fclose(fp);
    }

    // Create the disk image
    success = DiskImage::Create(
        path,
        sectorSize, sectorCount,
        headCount, sectorsPerTrack,
        sectorsPerCluster,
        mediaType, driveNumber,
        fatCount, fatWidth,
        rootCapacity,
        reservedCount, hiddenCount,
        volumeId, label);

    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}

int Help(const Command *cmd, const CommandArgs *args)
{
    if (args->Argc < 2)
    {
        PrintGlobalHelp();
    }
    else
    {
        const Command *cmdToGetHelpAbout = FindCommand(args->Argv[1]);
        if (!cmdToGetHelpAbout)
        {
            BAD_COMMAND(args->Argv[1]);
            return STATUS_ERROR;
        }

        PrintCommandHelp(cmdToGetHelpAbout);
        if (cmdToGetHelpAbout == cmd)
        {
            // Special case for 'help' command to print all valid commands
            const Command *cmds = GetCommands();
            int count = GetCommandCount();
            printf("\nCommands:\n");
            for (int i = 0; i < count; i++)
                printf("    %-16s%s\n", cmds[i].Name, cmds[i].Descripton);
        }
    }

    return STATUS_SUCCESS;
}
