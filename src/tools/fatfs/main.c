#include "command.h"
#include "image.h"

bool g_Verbose = false;
static CommandArgs s_CommandArgs = { 0 };
static bool s_PrintUsage = false;
static bool s_PrintVersionInfo = false;

static bool ParseCommandLine(int argc, char **argv);

int main(int argc, char **argv)
{
    if (!ParseCommandLine(argc, argv))
    {
        return STATUS_INVALIDARG;
    }

    if (s_PrintUsage)
    {
        PrintUsage();
        return STATUS_SUCCESS;
    }
    if (s_PrintVersionInfo)
    {
        PrintVersionInfo();
        return STATUS_SUCCESS;
    }

    const Command *cmd = FindCommand(s_CommandArgs.Argv[0]);
    if (!cmd)
    {
        LogError("invalid command - %s\n", s_CommandArgs.Argv[0]);
        return STATUS_ERROR;
    }

    return cmd->Func(&s_CommandArgs);
}

static bool ParseCommandLine(int argc, char **argv)
{
    // TODO: handle global --help

    int i;
    const char *opt;
    const char *optarg;
    const char *longopt;

    memset(&s_CommandArgs, 0, sizeof(s_CommandArgs));

#define GET_OPTARG(opt)                                 \
    optarg = argv[++i];                                 \
    if (argc <= i)                                      \
    {                                                   \
        LogError("missing argument for '%c'\n", opt);   \
        return false;                                   \
    }                                                   \

    i = 0;
    while (argc > ++i)
    {
        if (s_CommandArgs.Argc > 0)
        {
            // Stop processing once we've determined the command to execute.
            // Everything after command name are command arguments.
            break;
        }

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
                    break;
                }
                break;

            case '-':
                opt = &argv[i][0];
                while (opt && *(++opt))
                {
                    switch (*opt)
                    {
                        default:
                            LogError("invalid option - %c\n", *opt);
                            return false;
                        case '-':
                            longopt = ++opt;
                            opt = NULL;
                            if (strcmp(longopt, "help") == 0)
                            {
                                s_PrintUsage = true;
                                return true;
                            }
                            else if (strcmp(longopt, "version") == 0)
                            {
                                s_PrintVersionInfo = true;
                                return true;
                            }
                            LogError("invalid option - %s\n", longopt);
                            return false;
                        case 'v':
                            s_CommandArgs.Verbose = true;
                            break;
                    }
                }
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

void PrintUsage(void)
{
    const Command *cmds = GetCommands();
    int count = GetCommandCount();

    printf("Usage: %s [OPTIONS] DISKIMAGE COMMAND [ARGUMENTS]\n", PROG_NAME);
    printf("Create or manipulate the contents of a FAT-formatted disk image.\n");
    printf("For help about a specific command, run `%s x help COMMAND`.\n", PROG_NAME);
    printf("\n");
    printf("Options:\n");
    printf("    -v              Verbose output.\n");
    printf("    --help          Print this help menu and exit.\n");
    printf("    --version       Print program version information and exit.\n");
    printf("\n");
    printf("Commands:\n");
    for (int i = 0; i < count; i++)
        printf("    %-16s%s\n", cmds[i].Name, cmds[i].ShortHelp);
}

void PrintVersionInfo(void)
{
    printf("%s %s (%s)\n", PROG_NAME, PROG_VERSION, __DATE__);
    printf("Copyright (C) 2022 Wes Hampson\n");
}
