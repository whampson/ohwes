#include "command.h"
#include "diskimage.h"

#define STATUS_SUCCESS      0
#define STATUS_INVALIDARG   1
#define STATUS_ERROR        2

static int s_CommandArgc = 0;
static const char **s_CommandArgv = NULL;
static const char *s_CommandName = NULL;
static bool s_PrintUsage = false;
static const char *s_ImagePath = NULL;

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
    for (int i = 0; i < GetCommandCount(); i++)
        printf("  %s          %s\n", g_Commands[i].Name, g_Commands[i].Desc);
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

    const Command *cmd = FindCommand(s_CommandName);
    if (!cmd)
    {
        LogError("invalid command - %s\n", s_CommandName);
        return STATUS_ERROR;
    }

    cmd->CommandFunc(s_CommandArgc, s_CommandArgv);
    return STATUS_SUCCESS;
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
        if (s_CommandName)
        {
            // Stop processing once we've determined the command to execute.
            // Everything after command name are command arguments.
            break;
        }

        switch (argv[i][0])
        {
            default:
                if (!s_CommandName)
                {
                    s_CommandName = argv[i];
                    s_CommandArgc = argc - i - 1;
                    s_CommandArgv = &argv[i + 1];
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
                            s_ImagePath = optarg;
                            break;
                    }
                }
                break;
        }
    }

    if (!s_CommandName)
    {
        LogError("missing command\n");
        return false;
    }

    return true;
}
