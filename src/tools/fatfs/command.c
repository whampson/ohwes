#include <stdarg.h>
#include "command.h"

static int Help(int argc, const char **argv);
static int Info(int argc, const char **argv);

const Command g_Commands[] =
{
    { "help",
        "help [COMMAND]",
        "Print help info about this program or COMMAND.",
        NULL,
        Help
    },
    { "info",
        "info",
        "Print information about the disk image.",
        NULL,
        Info
    }
};

const Command * FindCommand(const char *name)
{
    for (int i = 0; i < GetCommandCount(); i++)
    {
        if (strcmp(g_Commands[i].Name, name) == 0)
        {
            return &g_Commands[i];
        }
    }

    return NULL;
}

int GetCommandCount(void)
{
    return (int) (sizeof(g_Commands) / sizeof(Command));
}

static int Help(int argc, const char **argv)
{
    if (argc == 0)
    {
        Usage();
        return 0;
    }

    const char *cmdName = argv[0];
    const Command *cmd = FindCommand(cmdName);
    if (cmd == NULL)
    {
        LogError("help: invalid command - %s\n", cmdName);
        return 1;
    }

    if (cmd->Usage)
    {
        printf("Usage: %s\n", cmd->Usage);
    }
    if (cmd->Desc)
    {
        printf("%s\n", cmd->Desc);
    }
    if (cmd->Help)
    {
        printf("\n%s\n", cmd->Help);
    }

    return 0;
}

static int Info(int argc, const char **argv)
{
    (void) argc;
    (void) argv;

    printf("DISK INFO\n");
    return 0;
}
