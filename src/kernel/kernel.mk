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

ifeq "${TEST_BUILD}" "1"
  SOURCES += \
    test/test_libc.c \
    test/printf_tests.c \
    test/string_tests.c
endif

TGT_CFLAGS := -Wno-unused-function

# TGT_INCDIRS := include
TGT_LDFLAGS := -Ttext 0x10000 -e KeEntry

TGT_LDLIBS  += \
    ${TARGET_DIR}/lib/libc.a \

TGT_PREREQS += lib/libc.a

# TODO: some kind of macro to add link libs in one step
TGT_LDLIBS  += \
    ${TARGET_DIR}/lib/libgcc.a \

TGT_PREREQS += lib/libgcc.a

TGT_POSTMAKE := $(call make-sys,${TARGETSYS})
# TODO: do this automatically when TARGETSYS is set?
