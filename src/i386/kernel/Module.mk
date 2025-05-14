TARGET         := lib/kernel/i386.a
TARGET_DEFINES := __KERNEL__
SOURCES := \
    entry.S \
    cpu.c \
    crash.c \
    debug.c \
    gdbstub.c \
    interrupt.c \
    pic.c \
    setup.S \
    timer.c \
    x86.c \
