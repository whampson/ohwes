TARGET := kernel/kernel.elf
TARGETSYS := sys/ohwes.sys

SOURCES := \
    entry.S \
    interrupt.S \
    console.c \
    handler.c \
    init.c \
    irq.c \
    vga.c \
    test.c \

TGT_CFLAGS := -Wno-unused-function

# TGT_INCDIRS := include
TGT_LDFLAGS := -Ttext 0x10000 -e KeEntry
TGT_LDLIBS  := \
    ${TARGET_DIR}/lib/libcrt.a \

TGT_POSTMAKE := $(call make-sys,${TARGETSYS})
# TODO: do this automatically when TARGETSYS is set?
