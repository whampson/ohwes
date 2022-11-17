#ifndef COMMAND_H
#define COMMAND_H

#include "fatfs.h"

typedef struct _CommandArgs
{
    int Argc;
    char * const *Argv;
} CommandArgs;

typedef struct _Command Command;

typedef struct _Command
{
    int (*Func)(const Command *cmd, const CommandArgs *args);
    const char *Name;
    const char *Usage;
    const char *ShortHelp;
    const char *LongHelp;
} Command;

const Command * GetCommands(void);
int GetCommandCount(void);

const Command * FindCommand(const char *name);
void PrintCommandHelp(const Command *cmd);

int Add(const Command *cmd, const CommandArgs *args);
int Attr(const Command *cmd, const CommandArgs *args);
int Create(const Command *cmd, const CommandArgs *args);
int Copy(const Command *cmd, const CommandArgs *args);
int Extract(const Command *cmd, const CommandArgs *args);
int Help(const Command *cmd, const CommandArgs *args);
int Info(const Command *cmd, const CommandArgs *args);
int List(const Command *cmd, const CommandArgs *args);
int Mkdir(const Command *cmd, const CommandArgs *args);
int Move(const Command *cmd, const CommandArgs *args);
int Remove(const Command *cmd, const CommandArgs *args);
int Rename(const Command *cmd, const CommandArgs *args);
int Touch(const Command *cmd, const CommandArgs *args);
int Type(const Command *cmd, const CommandArgs *args);

#endif  // COMMAND_H
