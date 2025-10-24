TARGET         := lib/kernel/i386.a
TARGET_DEFINES := __KERNEL__
SOURCES := \
    entry.S \
    cpu.c \
    crash.c \
    gdbstub.c \
    pgtbl.c \
    pic.c \
    pit.c \
    setup.S \
    x86.c \
