#include "command.h"
#include "image.h"

static const Command s_pCommands[] =
{
    { Add,
        "add", "add [OPTIONS] SOURCE[...]",
        "Add files to the disk image.",
        "SOURCE specifies the path to one or more files on the host system.\n"
        "Files are placed in the root directory unless -d is supplied.\n"
        "\n"
        "Options:\n"
        "    -d TARGETDIR    Add files to TARGETDIR.\n"
        "    -f              Overwrite files if they exist regardless of permissions.\n"
        "    -p              Create parent directories if they do not exist.\n"
    },
    { Attr,
        "attr", "attr [OPTIONS] FILE",
        "Change file attributes.",
        "Options:\n"
        "    -H              Set the 'hidden' bit.\n"
        "    -R              Set the 'read-only' bit.\n"
        "    -S              Set the 'system file' bit.\n"
        "    -h              Clear the 'hidden' bit.\n"
        "    -r              Clear the 'read-only' bit.\n"
        "    -s              Clear the 'system file' bit.\n"
        "    -X VALUE        Set the attributes byte to VALUE.\n"
    },
    { Copy,
        "copy", "copy [OPTIONS] SOURCE TARGET",
        "Copy the contents of a file or directory.",
        "Options:\n"
        "    -f              Overwrite TARGET if it exists regardless of permissions.\n"
        "    -p              Create parent directories if they do not exist.\n"
        "    -r              Copy subdirectories.\n"
    },
    { Create,
        "create", "create [OPTIONS]",
        "Create a new FAT-formatted disk image.",
        "DISKIMAGE will be used as the path to create the new disk image.\n"
        "\n"
        "Options:\n"
        "    -L              Set the volume label.\n"
        "    --force         Overwrite the disk image file if it exists.\n",
    },
    { Extract,
        "extract", "extract [OPTIONS] SOURCE[...]",
        "Extract files from the disk image.",
        "SOURCE specifies the path to one or more files on the disk image.\n"
        "Files are placed in the current working directory unless -d is supplied.\n"
        "\n"
        "Options:\n"
        "    -d TARGETDIR    Extract files to TARGETDIR.\n"
        "    -f              Overwrite files if they exist.\n"
        "    -r              Extract subdirectories.\n"
    },
    { Help,
        "help", "help COMMAND",
        "Print help about a command.",
        NULL,
    },
    { Info,
        "info", "info [FILE]",
        "Print file, directory, or disk information.",
        NULL,
    },
    { List,
        "list", "list [OPTIONS] FILE",
        "Print the contents of a directory.",
        "Options:\n"
        "    -a              List all files, including hidden files, volume labels,\n"
        "                    and device files.\n"
        "    -b              Bare format; print names only.\n"
        "    -r              Print the contents of subdirectories.\n"
    },
    { Mkdir,
        "mkdir", "mkdir [OPTIONS] PATH",
        "Create a directory.",
        "Options:\n"
        "    -p              Create parent directories if they do not exist.\n"
    },
    { Move,
        "move", "move [OPTIONS] SOURCE TARGET",
        "Move a file or directory to a new location.",
        "Options:\n"
        "    -f              Overwrite TARGET if it exists regardless of permissions.\n"
        "    -p              Create parent directories if they do not exist.\n"
    },
    { Remove,
        "remove", "remove [OPTIONS] FILE",
        "Remove a file or directory.",
        "Options:\n"
        "    -f              Remove PATH regardless of permissions or directory\n"
        "                    contents.\n"
    },
    { Rename,
        "rename", "rename FILE NEWNAME",
        "Rename a file or directory.",
        NULL,
    },
    { Touch,
        "touch", "touch [OPTIONS] FILE",
        "Change file access and modification times.",
        "By default, both the access and modification times are updated to the current\n"
        "date and time. If FILE does not exist, it will be created as an empty file.\n"
        "\n"
        "Options:\n"
        "    -a              Change the access time. Modification time is not updated\n"
        "                    unless -m is specified.\n"
        "    -m              Change the modification time. Access time is not updated\n"
        "                    unless -a is specified.\n"
        "    -d MMDDYYYY     Set access and/or modification date to MMDDYYYY.\n"
        "                        MM      Month, from 01 to 12.\n"
        "                        DD      Day, from 01 to 31.\n"
        "                        YYYY    Year, from 1980 to 2107.\n"
        "    -t hh:mm:ss     Set access and/or modification time to hh:mm:ss.\n"
        "                        hh      Hours, from 00 to 23.\n"
        "                        mm      Minutes, from 00 to 59.\n"
        "                        ss      Seconds, from 00 to 59.\n"
        "    -f              Update access and modification times regardless of\n"
        "                    permissions.\n"
    },
    { Type,
        "type", "type FILE",
        "Print the contents of a file.",
        NULL,
    }
};

const Command * GetCommands(void)
{
    return s_pCommands;
}

int GetCommandCount(void)
{
    return (int) (sizeof(s_pCommands) / sizeof(Command));
}

const Command * FindCommand(const char *name)
{
    for (int i = 0; i < GetCommandCount(); i++)
    {
        if (strcmp(s_pCommands[i].Name, name) == 0)
        {
            return &s_pCommands[i];
        }
    }

    return NULL;
}

int Add(const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Attr(const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Create(const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Copy(const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Extract(const CommandArgs *args)
{
    return STATUS_ERROR;
}

int List(const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Mkdir(const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Move(const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Remove(const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Rename(const CommandArgs *args)
{
    return STATUS_ERROR;
}

int Touch(const CommandArgs *args)
{
    return STATUS_ERROR;
}
