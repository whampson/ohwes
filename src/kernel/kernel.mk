TARGET := kernel/kernel.elf
TARGETSYS := sys/ohwes.sys

SOURCES := \
    entry.S \
    interrupt.S \
    console.c \
    cpu.c \
    handler.c \
    init.c \
    irq.c \
    vga.c \
    test.c \

TGT_CFLAGS := -Wno-unused-function

# TGT_INCDIRS := include
TGT_LDFLAGS := -Ttext 0x10000 -e KeEntry

TGT_LDLIBS  += \
    ${TARGET_DIR}/lib/libk.a \

TGT_PREREQS += lib/libk.a

# TODO: some kind of macro to add link libs in one step
TGT_LDLIBS  += \
    ${TARGET_DIR}/lib/libgcc.a \

TGT_PREREQS += lib/libgcc.a

ifeq "${TEST_BUILD}" "1"
  TGT_CFLAGS += -DTEST_BUILD
endif

TGT_POSTMAKE := $(call make-sys,${TARGETSYS})
# TODO: do this automatically when TARGETSYS is set?
