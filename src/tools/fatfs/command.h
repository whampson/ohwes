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

extern const Command g_Commands[];

const Command * FindCommand(const char *name);
int GetCommandCount(void);

#endif // __COMMAND_H
