#include "Command.hpp"
#include "DiskImage.hpp"

static const Command s_pCommands[] =
{
    { Create,
        "create", "create [OPTIONS] DISKIMAGE",
        "Create a new FAT-formatted disk image.",
        "  -l LABEL      Set the volume label to LABEL (11 chars max).\n"
        "  --force       Overwrite the disk image file if it already exists.",
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

    int bCustomLabel = 0;
    int bForce = 0;

    const char *path = NULL;

    char label[MAX_LABEL] = { };
    const char *tmpLabel = DEFAULT_LABEL;

    static struct option LongOptions[] =
    {
        { "force", no_argument, &bForce, 1 },
        { 0, 0, 0, 0 }
    };

    optind = 0;     // reset option index
    opterr = 0;     // prevent default error messages

    bool success = true;
    while (true)
    {
        int optIdx = 0;
        int c = getopt_long(args->Argc, args->Argv, "+:l:", LongOptions, &optIdx);

        if (c == -1 || !success)
            break;

        switch (c)
        {
            case 0:
                if (LongOptions[optIdx].flag != 0)
                    break;
                assert(!"unhandled getopt_long case!");
                break;
            case 'l':
                bCustomLabel = 1;
                tmpLabel = optarg;
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

    int len = strlen(tmpLabel);
    for (int i = 0; i < LABEL_LENGTH; i++)
    {
        if (i >= len)
            label[i] = ' ';
        else
            label[i] = tmpLabel[i];
    }

    if (optind < args->Argc)
    {
        path = args->Argv[optind];
    }

    if (!path)
    {
        LogError("missing disk image file name\n");
        return STATUS_INVALIDARG;
    }

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

    success = DiskImage::Create(path);
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
