TARGET         := lib/kernel/char.a
TARGET_DEFINES := __KERNEL__
TARGET_CFLAGS  := -Wno-unused-function

SOURCES := \
    char.c \
    ps2.c \
    ps2kb.c \
    rtc.c \
    serial.c \
    terminal.c \
    tty.c \
    tty_ldisc.c \
    vga.c \
