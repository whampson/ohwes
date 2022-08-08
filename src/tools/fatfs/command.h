#ifndef COMMAND_H
#define COMMAND_H

#include "fatfs.h"

typedef struct _CommandArgs
{
    int Argc;
    char * const *Argv;
    const char *ImagePath;
    bool Verbose;
} CommandArgs;

typedef struct _Command
{
    int (*Func)(const CommandArgs *args);
    const char *Name;
    const char *Usage;
    const char *ShortHelp;
    const char *LongHelp;
} Command;

const Command * GetCommands(void);
int GetCommandCount(void);

const Command * FindCommand(const char *name);

int Add(const CommandArgs *args);
int Attr(const CommandArgs *args);
int Create(const CommandArgs *args);
int Copy(const CommandArgs *args);
int Extract(const CommandArgs *args);
int Help(const CommandArgs *args);
int Info(const CommandArgs *args);  // like 'stat'
int List(const CommandArgs *args);
int Mkdir(const CommandArgs *args);
int Move(const CommandArgs *args);
int Remove(const CommandArgs *args);
int Rename(const CommandArgs *args);
int Touch(const CommandArgs *args);
int Type(const CommandArgs *args);

#endif  // COMMAND_H
