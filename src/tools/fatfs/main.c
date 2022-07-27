#include "command.h"
#include "image.h"

const char *s_pImagePath = NULL;
static int s_CommandArgc = 0;
static const char **s_ppCommandArgv = NULL;
static const char *s_pCommandName = NULL;
static bool s_PrintUsage = false;

static bool ParseCommandLine(int argc, const char **argv);

void Usage(void)
{
    printf("Usage: %s [OPTIONS] COMMAND [ARGS]\n", PROG_NAME);
    printf("FAT FileSystem utility. Created by Wes Hampson.\n");
    printf("Run `%s help COMMAND` to get help about a specific command.\n", PROG_NAME);
    printf("\n");
    printf("Options:\n");
    printf("  -i FILE       Specify the disk image file to operate on.\n");
    printf("  --help        Print this help menu and exit.\n");
    printf("\n");
    printf("Commands:\n");

    const Command *cmds = GetCommands();
    for (int i = 0; i < GetCommandCount(); i++)
        printf("  %s          %s\n", cmds[i].Name, cmds[i].Desc);
}

int main(int argc, const char **argv)
{
    if (!ParseCommandLine(argc, argv))
    {
        return STATUS_INVALIDARG;
    }
    if (s_PrintUsage)
    {
        Usage();
        return STATUS_SUCCESS;
    }

    if (s_pImagePath != NULL)
    {
        OpenImage(s_pImagePath);
    }

    const Command *cmd = FindCommand(s_pCommandName);
    if (!cmd)
    {
        LogError("invalid command - %s\n", s_pCommandName);
        return STATUS_ERROR;
    }

    int retval = cmd->CommandFunc(s_CommandArgc, s_ppCommandArgv);

    if (IsImageOpen())
    {
        CloseImage();
    }

    return retval;
}

static bool ParseCommandLine(int argc, const char **argv)
{
    int i;
    const char *opt;
    const char *optarg;
    const char *longopt;

#define GET_OPTARG(opt)                                 \
    optarg = argv[++i];                                 \
    if (argc <= i)                                      \
    {                                                   \
        LogError("missing argument for '%c'\n", opt);   \
        return false;                                   \
    }                                                   \

    if (argc < 2)
    {
        Usage();
        return false;
    }

    i = 0;
    while (argc > ++i)
    {
        if (s_pCommandName)
        {
            // Stop processing once we've determined the command to execute.
            // Everything after command name are command arguments.
            break;
        }

        switch (argv[i][0])
        {
            default:
                if (!s_pCommandName)
                {
                    s_pCommandName = argv[i];
                    s_CommandArgc = argc - i - 1;
                    s_ppCommandArgv = &argv[i + 1];
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
                            else
                            {
                                LogError("invalid option - %s\n", longopt);
                                return false;
                            }
                            break;
                        case 'i':
                            GET_OPTARG('i');
                            s_pImagePath = optarg;
                            break;
                    }
                }
                break;
        }
    }

    if (!s_pCommandName)
    {
        LogError("missing command\n");
        return false;
    }

    return true;
}
