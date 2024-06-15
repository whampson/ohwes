TARGET := elf/kernel.elf
TARGETEXE := sys/ohwes.sys

SOURCES := \
    _crt.c \
    console.c \
    cpu.c \
    crash.c \
    entry.S \
    i8042.c \
    irq.c \
    main.c \
    memory.c \
    paging.c \
    pic.c \
    ps2kb.c \
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

TGT_ASFLAGS := -D__KERNEL__
TGT_CFLAGS  := -D__KERNEL__ -Wno-unused-function
TGT_LDFLAGS := -T kernel/kernel.ld
# we only want the .text section right now for our hacky kernel image
OBJCOPYFLAGS := --only-section=.text

LINKLIBS := \
    lib/libc.a \
    lib/libgcc.a \
    lib/libos.a \

$(eval $(call add-linklibs,${LINKLIBS}))
# TODO: do this automatically when LINKLIBS set?
TGT_POSTMAKE := $(call make-exe,${TARGETEXE},${OBJCOPYFLAGS})
# TODO: do this automatically when TARGETEXE is set?
