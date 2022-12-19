#include "Command.hpp"

int g_Prefix = false;
int g_Quiet = false;
int g_QuietAll = false;
int g_Verbosity = 0;
const char *g_ProgramName = PROG_NAME;

static CommandArgs s_CommandArgs = { };
static int s_ShowHelp = false;
static int s_ShowVersion = false;

void PrintGlobalHelp()
{
    const Command *cmds = GetCommands();
    int count = GetCommandCount();

    printf("Usage: " PROG_NAME " [OPTIONS] COMMAND [ARGS]\n");
    printf("\n");
    printf("Commands:\n");
    for (int i = 0; i < count; i++) {
        printf("    %-16s%s\n", cmds[i].Name, cmds[i].Description);
    }
    printf("\n");
    printf("Options:\n");
    printf("    -p, --prefix    Prefix output with the program name.\n");
    printf("    -q, --quiet     Do not output informational messages (overrides -v).\n");
    printf("                      Errors and warnings will still be printed.\n");
    printf("    -Q, --quiet-all Do not output any messages (overrides -v).\n");
    printf("    -v, --verbose   Make the operation more talkative.\n");
    printf("        --help      Display this help message and exit.\n");
    printf("        --version   Display version information and exit.\n");
    printf("\n");
    printf("Run `" PROG_NAME " help COMMAND` to get help about a specific command.\n");
}

static int PrintHelp()
{
    if (s_CommandArgs.Argv == NULL) {
        PrintGlobalHelp();
        return STATUS_SUCCESS;
    }

    const Command *cmd = FindCommand(s_CommandArgs.Argv[0]);
    if (!cmd) {
        LogError_BadCommand(s_CommandArgs.Argv[0]);
        return STATUS_ERROR;
    }

    PrintCommandHelp(cmd);
    return STATUS_SUCCESS;
}

static int PrintVersion()
{
    printf("%s %s (%s)\n", PROG_NAME, PROG_VERSION, __DATE__);
    printf(PROG_COPYRIGHT "\n");

    return STATUS_SUCCESS;
}

static bool ParseCommandLine(int argc, char **argv)
{
    static struct option LongOptions[] = {
        { "prefix",     no_argument, 0, 'p' },
        { "quiet",      no_argument, 0, 'q' },
        { "quiet-all",  no_argument, 0, 'Q' },
        { "verbose",    no_argument, 0, 'v' },
        { "help",       no_argument, &s_ShowHelp, 1 },
        { "version",    no_argument, &s_ShowVersion, 1 },
        { 0, 0, 0, 0 }
    };

    g_ProgramName = GetFileName(argv[0]);

    optind = 0;     // reset option index
    opterr = 0;     // prevent default error messages

    bool success = true;
    while (true) {
        int optIdx = 0;
        int c = getopt_long(argc, argv, "+:pqQv", LongOptions, &optIdx);

        if (c == -1 || !success)
            break;

        switch (c) {
            case 0:
                if (LongOptions[optIdx].flag != 0)
                    break;
                assert(!"unhandled getopt_long case!");
                break;
            case 'p':
                g_Prefix = true;
                break;
            case 'q':
                g_Quiet = true;
                break;
            case 'Q':
                g_Quiet = true;
                g_QuietAll = true;
                break;
            case 'v':
                g_Verbosity++;
                break;
            case '?':
                if (optopt != 0)
                    LogError_BadOpt(optopt);
                else
                    LogError_BadLongOpt(&argv[optind - 1][2]);  // remove leading '--'
                success = false;
                break;
            default:
                assert(!"unhandled getopt_long case!");
                break;
        }
    }

    if (g_Quiet || g_QuietAll) {
        g_Verbosity = 0;
    }

    if (optind < argc) {
        s_CommandArgs.Argc = (argc - optind);
        s_CommandArgs.Argv = &argv[optind];
    }

    return success;
}

int main(int argc, char **argv)
{
    if (!ParseCommandLine(argc, argv))
        return STATUS_INVALIDARG;

    if (s_ShowHelp)
        return PrintHelp();

    if (s_ShowVersion)
        return PrintVersion();

    if (s_CommandArgs.Argc == 0) {
        LogError_MissingCommand();
        return STATUS_INVALIDARG;
    }

    const Command *cmd = FindCommand(s_CommandArgs.Argv[0]);
    if (!cmd) {
        LogError_BadCommand(s_CommandArgs.Argv[0]);
        return STATUS_ERROR;
    }

    int status = cmd->Func(cmd, &s_CommandArgs);
    if (status != STATUS_SUCCESS) {
        LogVerbose("'%s' failed with exit code %d\n", cmd->Name, status);
    }

    return status;
}
