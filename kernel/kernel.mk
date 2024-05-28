TARGET := kernel/kernel.elf
TARGETSYS := sys/ohwes.sys

# entry.S MUST be first so the entrypoint is at a known location
SOURCES := \
    entry.S \
    console.c \
    cpu.c \
    crash.c \
    i8042.c \
    irq.c \
    main.c \
    memory.c \
    pic.c \
    ps2kb.c \
    queue.c \
    rtc.c \
    syscall.c \
    task.c \
    timer.c \
    vga.c \
    sys/open.c \

ifeq "${TEST_BUILD}" "1"
  SOURCES += \
    test/tests.c \
    test/printf_tests.c \
    test/string_tests.c \
    test/queue_tests.c \
    test/syscall_tests.c \

endif

TGT_CFLAGS  := -Wno-unused-function
TGT_LDFLAGS := -Ttext 0x20000 -e kentry

LINKLIBS := \
    lib/libc.a \
    lib/libgcc.a \
    lib/init.a \
    # TODO: make init.a into executable

$(eval $(call add-linklibs,${LINKLIBS}))
# TODO: do this automatically when LINKLIBS set?
TGT_POSTMAKE := $(call make-sys,${TARGETSYS})
# TODO: do this automatically when TARGETSYS is set?
