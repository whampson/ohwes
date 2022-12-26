#ifndef COMMAND_HPP
#define COMMAND_HPP

#include "fatfs.hpp"

#define CheckParam(x,...)                                                       \
do {                                                                            \
    if (!(x)) { LogError(__VA_ARGS__); return STATUS_INVALIDARG; }              \
} while (0)

struct CommandArgs {
    int Argc;
    char * const *Argv;
};

struct Command;
struct Command {
    int (*Func)(const Command *cmd, const CommandArgs *args);
    const char *Name;
    const char *Synopsis;
    const char *Description;
    const char *Options;
};

const Command * GetCommands(void);
int GetCommandCount(void);

const Command * FindCommand(const char *name);
int PrintCommandHelp(const Command *cmd);

// int Add(const Command *cmd, const CommandArgs *args);
// int Attr(const Command *cmd, const CommandArgs *args);
int Create(const Command *cmd, const CommandArgs *args);
// int Copy(const Command *cmd, const CommandArgs *args);
// int Extract(const Command *cmd, const CommandArgs *args);
int Help(const Command *cmd, const CommandArgs *args);
int Info(const Command *cmd, const CommandArgs *args);
int List(const Command *cmd, const CommandArgs *args);
// int Mkdir(const Command *cmd, const CommandArgs *args);
// int Move(const Command *cmd, const CommandArgs *args);
// int Remove(const Command *cmd, const CommandArgs *args);
// int Rename(const Command *cmd, const CommandArgs *args);
// int Touch(const Command *cmd, const CommandArgs *args);
// int Type(const Command *cmd, const CommandArgs *args);

int Test(const Command *cmd, const CommandArgs *args);

#endif  // COMMAND_HPP
