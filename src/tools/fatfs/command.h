#ifndef __COMMAND_H
#define __COMMAND_H

#include "fatfs.h"

typedef struct _Command
{
    const char *Name;
    const char *Usage;
    const char *Desc;
    const char *Help;
    int (*CommandFunc)(int argc, const char **argv);
} Command;

const Command * GetCommands(void);
int GetCommandCount(void);

const Command * FindCommand(const char *name);

#endif // __COMMAND_H
