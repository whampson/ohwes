#include "Command.hpp"

int g_bVerbose = false;

static CommandArgs s_CommandArgs = { };
static int s_bShowHelp = false;
static int s_bShowVersion = false;

void PrintGlobalHelp()
{
    const Command *cmds = GetCommands();
    int count = GetCommandCount();

    printf("Usage: " PROG_NAME " [OPTIONS] COMMAND [ARGS]\n");
    printf("\n");
    printf("Commands:\n");
    for (int i = 0; i < count; i++)
        printf("    %-16s%s\n", cmds[i].Name, cmds[i].Descripton);
    printf("\n");
    printf("Options:\n");
    printf("    -v, --verbose   Make the operation more talkative.\n");
    printf("    -?, --help      Display this help message and exit.\n");
    printf("    --version       Display version information and exit.\n");
    printf("\n");
    printf("Run `fatfs help COMMAND` to get help about a specific command.\n");
}

static int PrintHelp()
{
    if (s_CommandArgs.Argv == NULL)
    {
        PrintGlobalHelp();
        return STATUS_SUCCESS;
    }

    const Command *cmd = FindCommand(s_CommandArgs.Argv[0]);
    if (!cmd)
    {
        BAD_COMMAND(s_CommandArgs.Argv[0]);
        return STATUS_ERROR;
    }

    PrintCommandHelp(cmd);
    return STATUS_SUCCESS;
}

static int PrintVersion()
{
    printf("%s %s (%s)\n", PROG_NAME, PROG_VERSION, __DATE__);
    printf("Copyright (C) 2022 Wes Hampson\n");

    return STATUS_SUCCESS;
}

static bool ParseCommandLine(int argc, char **argv)
{
    static struct option LongOptions[] =
    {
        { "help", no_argument, &s_bShowHelp, 1 },
        { "version", no_argument, &s_bShowVersion, 1 },
        { "verbose", no_argument, &g_bVerbose, 1 },
        { 0, 0, 0, 0 }
    };

    optind = 0;     // reset option index
    opterr = 0;     // prevent default error messages

    bool success = true;
    while (true)
    {
        int optIdx = 0;
        int c = getopt_long(argc, argv, "+v", LongOptions, &optIdx);

        if (c == -1 || !success)
            break;

        switch (c)
        {
            case 0:
                if (LongOptions[optIdx].flag != 0)
                    break;
                assert(!"unhandled getopt_long case!");
                break;
            case 'v':
                g_bVerbose = true;
                break;
            case '?':
                if (optopt != 0)
                    BAD_OPT(optopt);
                else
                    BAD_LONGOPT(&argv[optind - 1][2]);  // remove leading '--'
                success = false;
                break;
            default:
                assert(!"unhandled getopt_long case!");
                break;
        }
    }

    if (optind < argc)
    {
        s_CommandArgs.Argc = (argc - optind);
        s_CommandArgs.Argv = &argv[optind];
    }

    return success;
}

int main(int argc, char **argv)
{
    if (!ParseCommandLine(argc, argv))
    {
        return STATUS_INVALIDARG;
    }

    if (s_bShowHelp)
    {
        return PrintHelp();
    }
    if (s_bShowVersion)
    {
        return PrintVersion();
    }

    if (s_CommandArgs.Argc == 0)
    {
        MISSING_COMMAND();
        return STATUS_INVALIDARG;
    }

    const Command *cmd = FindCommand(s_CommandArgs.Argv[0]);
    if (!cmd)
    {
        BAD_COMMAND(s_CommandArgs.Argv[0]);
        return STATUS_ERROR;
    }

    int status = cmd->Func(cmd, &s_CommandArgs);
    if (status != STATUS_SUCCESS)
    {
        LogVerbose("'%s' failed with exit code %d.\n", cmd->Name, status);
    }

    return status;
}
