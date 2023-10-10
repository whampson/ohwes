TARGET := kernel/kernel.elf
TARGETSYS := sys/ohwes.sys

SOURCES := \
    entry.S \
    interrupt.S \
    console.c \
    debug.c \
    handler.c \
    init.c \
    irq.c \

# TGT_INCDIRS := include
TGT_LDFLAGS := -Ttext 0x100000 -e KeEntry
TGT_LDLIBS  := \
    ${TARGET_DIR}/lib/libcrt.a \

TGT_POSTMAKE := $(call make-sys,${TARGETSYS})
# TODO: do this automatically when TARGETSYS is set?
