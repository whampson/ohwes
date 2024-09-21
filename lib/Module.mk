TARGET  := libc.a
SOURCES := \
    ctype.c \
    errno.c \
    printf.c \
    queue.c \
    stdio.c \
    string.c \
    syscall.c \

TARGET_CFLAGS  := -nostdinc -ffreestanding
