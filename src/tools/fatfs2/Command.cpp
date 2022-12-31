#include "Command.hpp"
#include "FatDisk.hpp"

static const Command s_pCommands[] = {
    { Attr,
        "attr", PROG_NAME " attr DISK [OPTIONS] FILE",
        "View or change file attributes",
        "  -a, -A            Set/clear the ARCHIVE bit\n"
        "  -h, -H            Set/clear the HIDDEN bit\n"
        "  -l, -L            Set/clear the VOLUME_LABEL bit\n"
        "  -r, -R            Set/clear the READONLY bit\n"
        "  -s, -S            Set/clear the SYSTEM bit\n"
        "  -v, -V            Set/clear the DEVICE bit\n"
        "  --offset=SECTOR   Read the file system from a specific sector on disk\n"
    },
    { Create,   // similar to mkdosfs
        "create", PROG_NAME " create [OPTIONS] DISK [NSECTORS]",
        "Create a new FAT file system on the specified disk",
        "  -d NUMBER         Set the drive number to NUMBER\n"
        "  -f COUNT          Create COUNT file allocation tables\n"
        "  -F WIDTH          Select the FAT width (12, or 16)\n"
        "  -g HEADS/SPT      Select the disk geometry (as heads/sectors_per_track)\n"
        "  -i VOLID          Set the volume ID to VOLID (as a 32-bit hex number)\n"
        "  -l LABEL          Set the volume label to LABEL (11 chars max)\n"
        "  -m TYPE           Set the media type ID to TYPE\n"
        "  -r COUNT          Create space for at least COUNT root directory entries\n"
        "  -R COUNT          Create COUNT reserved sectors\n"
        "  -s COUNT          Set the number of logical sectors per cluster to COUNT\n"
        "  -S SIZE           Set the logical sector size to SIZE (power of 2, minimum 512)\n"
        "  --force           Overwrite the disk image file if it already exists\n"
        "  --no-align        Disable structure alignment\n"
        "  --offset=SECTOR   Write the file system to a specific sector on disk\n"
    },
    { Extract,
        "extract", PROG_NAME " extract DISK FILE [TARGET]",
        "Extract a file from the disk",
        // TODO: -r
        "  --force           Overwrite the target file if it already exists\n"
    },
    { Help,
        "help", PROG_NAME " help [COMMAND]",
        "Print the help menu for this tool or a specific command",
        NULL
    },
    { Info,
        "info", PROG_NAME " info [OPTIONS] DISK [FILE]",
        "Print information about a disk or a file on disk",
        "  --offset=SECTOR   Read the file system from a specific sector on disk\n"
    },
    { List,
        "list", PROG_NAME " list [OPTIONS] DISK [FILE]",
        "Print the contents of a directory",
        "  -a                List all files; include hidden files and volume labels\n"
        "  -A                Show file attributes\n"
        "  -b                Bare format; print file names only\n"
        "  -n                Show short names only\n"
        // "  -r                List the contents of subdirectories\n"
        "  -s                Show file allocation size\n"
        "  --offset=SECTOR   Read the file system from a specific sector on disk\n"
    },
    { Test,
        "test", PROG_NAME " test",
        "Run the test suite",
        NULL
    },
    { Touch,
        "touch", PROG_NAME " touch DISK [OPTIONS] FILE",
        "Change file access and/or modification times",
        "  -a                Change file access time only\n"
        "  -m                Change file modification time only\n"
    },
    { Type,
        "type", PROG_NAME " type DISK FILE",
        "Print the contents of a file",
        NULL
    },
};

const Command * GetCommands()
{
    return s_pCommands;
}

int GetCommandCount()
{
    return (int) (sizeof(s_pCommands) / sizeof(Command));
}

const Command * FindCommand(const char *name)
{
    for (int i = 0; i < GetCommandCount(); i++) {
        if (strcmp(s_pCommands[i].Name, name) == 0) {
            return &s_pCommands[i];
        }
    }

    return NULL;
}

int PrintCommandHelp(const Command *cmd)
{
    if (cmd->Synopsis)
        printf("Usage: %s\n", cmd->Synopsis);

    if (cmd->Description)
        printf("%s.\n", cmd->Description);

    if (cmd->Options)
        printf("\nOptions:\n%s\n", cmd->Options);

    return STATUS_SUCCESS;
}

int Help(const Command *cmd, const CommandArgs *args)
{
    if (args->Argc < 2) {
        PrintGlobalHelp();
        return STATUS_SUCCESS;
    }

    const Command *cmdToGetHelpAbout = FindCommand(args->Argv[1]);
    if (!cmdToGetHelpAbout) {
        LogError_BadCommand(args->Argv[1]);
        return STATUS_ERROR;
    }

    PrintCommandHelp(cmdToGetHelpAbout);
    if (cmdToGetHelpAbout == cmd) {
        // Special case for 'help' command to print all valid commands
        const Command *cmds = GetCommands();
        int count = GetCommandCount();
        printf("\nCommands:\n");
        for (int i = 0; i < count; i++) {
            printf("  %-18s%s\n", cmds[i].Name, cmds[i].Description);
        }
        printf("\n");
    }
    return STATUS_SUCCESS;
}
