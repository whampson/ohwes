TARGET := kernel/kernel.elf

SOURCES := \
    entry.S \
    interrupt.S \
    console.c \
    debug.c \
    handler.c \
    init.c \
    irq.c \

TGT_INCDIRS := include
TGT_LDFLAGS := -Ttext 0x100000
TGT_LDLIBS  := \
    ${TARGET_DIR}/lib/libcrt.a \
