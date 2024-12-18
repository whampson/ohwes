#ifdef __ASSEMBLER__
#define SYSCALL_DECLARE(name)    .long sys_##name
#else
#define SYSCALL_DECLARE(name)    _SYS_##name,
enum _syscall_nr {
#endif


//
// System Call Numbers
//

SYSCALL_DECLARE(_exit)
SYSCALL_DECLARE(read)
SYSCALL_DECLARE(write)
SYSCALL_DECLARE(open)
SYSCALL_DECLARE(close)
SYSCALL_DECLARE(ioctl)
SYSCALL_DECLARE(dup)
SYSCALL_DECLARE(dup2)


#ifndef __ASSEMBLER__
NR_SYSCALLS
};
#endif

#undef SYSCALL_DECLARE
