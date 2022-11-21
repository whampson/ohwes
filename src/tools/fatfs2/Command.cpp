#include "Command.hpp"
#include "DiskImage.hpp"

static const Command s_pCommands[] =
{
    { Create,
        "create", "create [OPTIONS] DISKIMAGE",
        "Create a new FAT-formatted disk image.",
        "  -l LABEL      Set the volume label (11 chars max)\n"
        "  -m TYPE       Set the media type ID\n"
        "  --force       Overwrite the disk image file if it already exists",
        // TODO: more
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
    (void) cmd;

    const char *path = NULL;

    // Option booleans
    int bHelp = 0;
    int bForce = 0;
    int bLabel = 0;
    int bMediaType = 0;

    // Option variables, with defaults
    int mediaType = DEFAULT_MEDIA_TYPE;
    char label[MAX_LABEL] = { };
    const char *pLabel = DEFAULT_LABEL;

    const int fatWidth = 12;        // TODO: 16, 32

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
        int c = getopt_long(args->Argc, args->Argv, "+:l:m:", LongOptions, &optIdx);

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
            case 'l':
                bLabel = 1;
                pLabel = optarg;
                break;
            case 'm':
                bMediaType = 1;
                mediaType = strtol(optarg, NULL, 0);
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

    // Copy label, pad with spaces.
    int len = strlen(pLabel);
    for (int i = 0; i < LABEL_LENGTH; i++)
    {
        if (i >= len)
            label[i] = ' ';
        else
            label[i] = pLabel[i];
    }

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

    // Test whether the file exists, fail if it does and --force not specified
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
        path, label, fatWidth, mediaType);
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
