TARGET         := lib/kernel/i386.a
TARGET_DEFINES := __KERNEL__
SOURCES := \
    entry.S \
    cpu.c \
    crash.c \
    gdbstub.c \
    pic.c \
    setup.S \
    timer.c \
