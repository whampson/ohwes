#include "command.h"
#include "image.h"

bool g_Verbose = false;

static CommandArgs s_CommandArgs = { 0 };
static bool s_ShowHelp = false;
static bool s_ShowVersion = false;

static int PrintHelp(void)
{
    if (s_CommandArgs.Argv == NULL)
    {
        const Command *cmds = GetCommands();
        int count = GetCommandCount();

        printf("Usage: " PROG_NAME " [OPTIONS] IMAGE COMMAND [ARGS]\n");
        printf("\n");
        printf("Options:\n");
        printf("    -v              Make the operation more talkative.\n");
        printf("    --help          Display help and exit.\n");
        printf("    --version       Display version information and exit.\n");
        printf("\n");
        printf("Commands:\n");
        for (int i = 0; i < count; i++)
            printf("    %-16s%s\n", cmds[i].Name, cmds[i].ShortHelp);
        printf("\n");

        return STATUS_SUCCESS;
    }

    const Command *cmd = FindCommand(s_CommandArgs.Argv[0]);
    if (!cmd)
    {
        LogError("invalid command '%s'\n", s_CommandArgs.Argv[0]);
        return STATUS_ERROR;
    }

    if (cmd->Usage)
    {
        printf("Usage: %s\n", cmd->Usage);
    }
    if (cmd->ShortHelp)
    {
        printf("%s\n", cmd->ShortHelp);
    }
    if (cmd->LongHelp)
    {
        printf("\n%s", cmd->LongHelp);
    }

    return STATUS_SUCCESS;
}

static int PrintVersion(void)
{
    printf("%s %s (%s)\n", PROG_NAME, PROG_VERSION, __DATE__);
    printf("Copyright (C) 2022 Wes Hampson\n");

    return STATUS_SUCCESS;
}

static bool ParseCommandLine(int argc, char **argv)
{
    int i;
    const char *opt;
    const char *optarg;
    const char *longopt;
    bool gotCmd = false;

    // TODO: getopt_long?
    //       though, it serves as a good starting point for a getopt() impl.

    memset(&s_CommandArgs, 0, sizeof(s_CommandArgs));

/*
#define GET_OPTARG(opt)                                 \
{                                                       \
    optarg = argv[++i];                                 \
    if (argc <= i)                                      \
    {                                                   \
        LogError("missing argument for '%c'\n", opt);   \
        return false;                                   \
    }                                                   \
}
*/

    // This will locate '-?', '--help', and '--version' anywhere in the arg
    // string, and will validate all inputs before the 'IMAGE' and 'COMMAND'
    // arguments. Command-specific 'ARGUMENTS' is syntax validated but not
    // parsed for invalid argument errors; that is left up to the command impl.

    i = 0;
    while (argc > ++i)
    {
        switch (argv[i][0])
        {
        default:
            if (!s_CommandArgs.ImagePath)
            {
                s_CommandArgs.ImagePath = argv[i];
                break;
            }
            if (s_CommandArgs.Argc == 0)
            {
                s_CommandArgs.Argc = argc - i;
                s_CommandArgs.Argv = &argv[i];
                gotCmd = true;
                break;
            }
            break;

        case '-':
            opt = &argv[i][1];
            if (opt[0] == '\0')
            {
                LogError("missing option name\n");
                return false;
            }
            do
            {
                switch (*opt)
                {
                default:
                    if (gotCmd) break;
                    LogError("invalid option '%c'\n", *opt);
                    return false;
                case 'v':
                    if (gotCmd) break;
                    g_Verbose = true;
                    break;
                case '?':
                    s_ShowHelp = true;
                    break;
                case '-':
                    longopt = ++opt;
                    opt = NULL;
                    if (longopt[0] == '\0')
                    {
                        LogError("missing option name\n");
                        return false;
                    }
                    if (strcmp(longopt, "help") == 0)
                    {
                        s_ShowHelp = true;
                        return true;
                    }
                    else if (strcmp(longopt, "version") == 0)
                    {
                        s_ShowVersion = true;
                        return true;
                    }

                    if (gotCmd) break;
                    LogError("invalid option '%s'\n", longopt);
                    return false;
                }
            } while (*(++opt));
            break;
        }
    }

    if (!s_CommandArgs.ImagePath)
    {
        LogError("missing disk image\n");
        return false;
    }

    if (!s_CommandArgs.Argc)
    {
        LogError("missing command\n");
        return false;
    }

    return true;
}

int main(int argc, char **argv)
{
    if (!ParseCommandLine(argc, argv))
    {
        return STATUS_INVALIDARG;
    }

    if (s_ShowHelp)
    {
        return PrintHelp();
    }
    if (s_ShowVersion)
    {
        return PrintVersion();
    }

    const Command *cmd = FindCommand(s_CommandArgs.Argv[0]);
    if (!cmd)
    {
        LogError("invalid command '%s'\n", s_CommandArgs.Argv[0]);
        return STATUS_ERROR;
    }

    int status = cmd->Func(cmd, &s_CommandArgs);
    if (status != STATUS_SUCCESS)
    {
        LogError("%s: failed with exit code %d.\n", cmd->Name, status);
    }

    return status;
}
