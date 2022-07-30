#include "command.h"
#include "image.h"

int Help(const CommandArgs *args)
{
    if (args -> Argc == 0)
    {
        PrintUsage();
        return STATUS_SUCCESS;
    }

    const char *cmdName = args->Argv[0];
    const Command *cmd = FindCommand(cmdName);
    if (cmd == NULL)
    {
        LogError("help: invalid command - %s\n", cmdName);
        return STATUS_INVALIDARG;
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
