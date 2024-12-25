TARGET         := lib/kernel/chdev.a
TARGET_DEFINES := __KERNEL__
TARGET_CFLAGS  := -Wno-unused-function

SOURCES := \
    chdev.c \
    console.c \
    ps2.c \
    ps2kb.c \
    rtc.c \
    serial.c \
    tty.c \
    tty_ldisc.c \
    vga.c \
