#include "Command.hpp"

int g_bShowHelp = false;
int g_bShowVersion = false;
int g_bUsePrefix = false;
int g_nQuietness = 0;
int g_nVerbosity = 0;
uint32_t g_nSectorOffset = 0;

int _g_nAllocCount = 0;

const char *g_ProgramName = PROG_NAME;

int optidx = 0;

static CommandArgs s_CommandArgs = { };

// TODO: allow an environment variable to be set containing disk image
// for command-line brevity

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
    LogInfo("%s %s (%s)\n", PROG_NAME, PROG_VERSION, __DATE__);
    LogInfo(PROG_COPYRIGHT "\n");

    return STATUS_SUCCESS;
}

int PrintGlobalHelp()
{
    const Command *cmds = GetCommands();
    int count = GetCommandCount();

    LogInfo("Usage: " PROG_NAME " [OPTIONS] COMMAND [ARGS]\n");
    LogInfo("FAT File System disk image manipulation tool.\n");
    LogInfo("\nCommands:\n");
    for (int i = 0; i < count; i++) {
        LogInfo("  %-18s%s\n", cmds[i].Name, cmds[i].Description);
    }
    LogInfo("\nGlobal Options:\n");
    LogInfo("  -p, --prefix      Prefix output with the program name\n");
    LogInfo("  -q, --quiet       Suppress informational output, keep errors (overrides -v)\n");
    LogInfo("      --quiet-all   Suppress all output (overrides -v)\n");
    LogInfo("  -v, --verbose     Make the operation more talkative\n");
    LogInfo("      --very-verbose Extra chatty output\n");
    LogInfo("      --help        Display this help message and exit\n");
    LogInfo("      --version     Display version information and exit\n");
    LogInfo("\nGlobal long options can be supplied anywhere in the argument string, while their\n");
    LogInfo("short option counterparts can only occur before COMMAND. This is done to avoid\n");
    LogInfo("potential conflicts with command-specific options.\n");
    LogInfo("\nTo get help about a command, run `" PROG_NAME " help COMMAND` or `" PROG_NAME " COMMAND --help`.\n");

    // TODO: --offset=SECTOR
    // TODO: print these with command-specific help w/o the short opts

    return STATUS_SUCCESS;
}

void FormatDate(char dst[MAX_DATE], const struct tm *src)
{
    char month[4];
    switch (src->tm_mon)
    {
        case 0: sprintf(month, "Jan"); break;
        case 1: sprintf(month, "Feb"); break;
        case 2: sprintf(month, "Mar"); break;
        case 3: sprintf(month, "Apr"); break;
        case 4: sprintf(month, "May"); break;
        case 5: sprintf(month, "Jun"); break;
        case 6: sprintf(month, "Jul"); break;
        case 7: sprintf(month, "Aug"); break;
        case 8: sprintf(month, "Sep"); break;
        case 9: sprintf(month, "Oct"); break;
        case 10:sprintf(month, "Nov"); break;
        case 11:sprintf(month, "Dec"); break;
        default: sprintf(month, "   "); break;
    }

    snprintf(dst, MAX_DATE, "%3s %2d %4d",
        month, src->tm_mday, src->tm_year + 1900);
}

void FormatTime(char dst[MAX_TIME], const struct tm *src)
{
    snprintf(dst, MAX_TIME, "%02d:%02d",
        src->tm_hour, src->tm_min);
}

int main(int argc, char **argv)
{
    g_ProgramName = GetFileName(argv[0]);

    static struct option LongOptions[] = {
        GLOBAL_LONGOPTS,
        { 0, 0, 0, 0 }
    };

    optind = 0;     // getopt: reset option index
    opterr = 0;     // getopt: prevent default error messages
    optidx = 0;     // reset option index

    // Parse option arguments
    while (true) {
        int c = getopt_long(
            argc, argv,
            "+:pqv",
            LongOptions, &optidx);

        if (c == -1) break;
        ProcessGlobalOption(argv, LongOptions, c);

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
    }

    if (optind < argc) {
        s_CommandArgs.Argc = (argc - optind);
        s_CommandArgs.Argv = &argv[optind];
    }
    if (s_CommandArgs.Argc == 0) {
        LogError_MissingCommand();
        LogInfo("Run `" PROG_NAME " --help' for help.\n");
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

    if (_g_nAllocCount != 0) {
        LogWarning("%d UNFREE'D ALLOCS!\n", _g_nAllocCount);
    }

    return status;
}
