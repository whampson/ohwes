#include "Command.hpp"

int g_bShowHelp = false;
int g_bShowVersion = false;
int g_bUsePrefix = false;
int g_nQuietness = 0;
int g_nVerbosity = 0;

const char *g_ProgramName = PROG_NAME;

static CommandArgs s_CommandArgs = { };

int PrintHelp()
{
    if (s_CommandArgs.Argv == NULL) {
        return PrintGlobalHelp();
    }

    const Command *cmd = FindCommand(s_CommandArgs.Argv[0]);
    if (!cmd) {
        LogError_BadCommand(s_CommandArgs.Argv[0]);
        return STATUS_ERROR;
    }

    PrintCommandHelp(cmd);
    return STATUS_SUCCESS;
}

int PrintVersion()
{
    printf("%s %s (%s)\n", PROG_NAME, PROG_VERSION, __DATE__);
    printf(PROG_COPYRIGHT "\n");

    return STATUS_SUCCESS;
}

int PrintGlobalHelp()
{
    const Command *cmds = GetCommands();
    int count = GetCommandCount();

    printf("Usage: " PROG_NAME " [OPTIONS] COMMAND [ARGS]\n");
    printf("\n");
    printf("Commands:\n");
    for (int i = 0; i < count; i++) {
        printf("    %-16s%s\n", cmds[i].Name, cmds[i].Description);
    }
    printf("Options:\n");
    printf("    -p, --prefix    Prefix output with the program name.\n");
    printf("    -q, --quiet     Suppress output (overrides -v).\n");
    printf("    -v, --verbose   Make the operation more talkative.\n");
    printf("        --help      Display this help message and exit.\n");
    printf("        --version   Display version information and exit.\n");
    printf("\n");
    printf("Run `" PROG_NAME " help COMMAND` to get help about a specific command.\n");

    return STATUS_SUCCESS;
}

bool ProcessGlobalOption(int c)
{
    switch (c) {
        case 'p':
            g_bUsePrefix = true;
            break;
        case 'q':
            g_nQuietness += 1;
            break;
        case 'v':
            g_nVerbosity += 1;
            break;
    }

    if (g_bShowHelp) {
        PrintHelp();
        return true;
    }

    if (g_bShowVersion) {
        PrintVersion();
        return true;
    }

    return false;;
}

int main(int argc, char **argv)
{
    g_ProgramName = GetFileName(argv[0]);

    static struct option LongOptions[] = {
        GLOBAL_LONGOPTS,
        { 0, 0, 0, 0 }
    };

    optind = 0;     // reset option index
    opterr = 0;     // prevent default error messages

    // Parse option arguments
    while (true) {
        int optIdx = 0;
        int c = getopt_long(
            argc, argv,
            GLOBAL_OPTSTRING,
            LongOptions, &optIdx);

        if (ProcessGlobalOption(c)) {
            return STATUS_SUCCESS;
        }
        if (c == -1) break;

        switch (c) {
            case '?':
                if (optopt != 0)
                    LogError_BadOpt(optopt);
                else
                    LogError_BadLongOpt(&argv[optind - 1][2]);  // remove leading '--'
                return STATUS_INVALIDARG;
                break;
            case 0:
                if (LongOptions[optIdx].flag != 0)
                    break;
                assert(!"unhandled getopt_long case!");
                break;
        }
    }

    if (optind < argc) {
        s_CommandArgs.Argc = (argc - optind);
        s_CommandArgs.Argv = &argv[optind];
    }
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
