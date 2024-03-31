TARGET := kernel/kernel.elf
TARGETSYS := sys/ohwes.sys

# entry.S MUST be first so the entrypoint is at a known location
SOURCES := \
    entry.S \
    console.c \
    cpu.c \
    crash.c \
    i8042.c \
    init.c \
    irq.c \
    main.c \
    memory.c \
    pic.c \
    ps2kb.c \
    queue.c \
    syscall.c \
    task.c \
    timer.c \
    vga.c \

ifeq "${TEST_BUILD}" "1"
  SOURCES += \
    test/tests.c \
    test/printf_tests.c \
    test/string_tests.c \
    test/queue_tests.c
endif

TGT_CFLAGS  := -Wno-unused-function
TGT_LDFLAGS := -Ttext 0x10000 -e kentry

LINKLIBS := \
    lib/libc.a \
    lib/libgcc.a \

$(eval $(call add-linklibs,${LINKLIBS}))
# TODO: do this automatically when LINKLIBS set?
TGT_POSTMAKE := $(call make-sys,${TARGETSYS})
# TODO: do this automatically when TARGETSYS is set?
