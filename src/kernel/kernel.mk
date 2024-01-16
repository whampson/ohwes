TARGET := kernel/kernel.elf
TARGETSYS := sys/ohwes.sys

SOURCES := \
    entry.S \
    console.c \
    cpu.c \
    interrupt.c \
    init.c \
    irq.c \
    syscall.c \
    vga.c \

ifeq "${TEST_BUILD}" "1"
  SOURCES += \
    test/test_libc.c \
    test/printf_tests.c \
    test/string_tests.c
endif

TGT_CFLAGS  := -Wno-unused-function -Wno-noreturn
TGT_LDFLAGS := -Ttext 0x10000 -e kentry

LINKLIBS := \
    lib/libc.a \
    lib/libgcc.a \

$(call add-linklibs,${LINKLIBS})
# TODO: do this automatically when LINKLIBS set?
TGT_POSTMAKE := $(call make-sys,${TARGETSYS})
# TODO: do this automatically when TARGETSYS is set?
