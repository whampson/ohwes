#include <errno.h>
#include <syscall.h>

int _errno; // TODO: this needs to point to errno in the current task structure

// syscall user-mode function definitions, TODO: move these elsewhere
SYSCALL_FN1_VOID(exit, int, status)
SYSCALL_FN3(int, read, int, fd, void *, buf, size_t, count)
SYSCALL_FN3(int, write, int, fd, const void *, buf, size_t, count)
SYSCALL_FN2(int, open, const char *, name, int, flags)
SYSCALL_FN1(int, close, int, fd)
SYSCALL_FN3(int, ioctl, int, fd, unsigned int, cmd, void *, arg)
