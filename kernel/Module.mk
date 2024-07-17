TARGET := kernel.elf
TARGET_EXE := ohwes.sys

SOURCES := \
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

TARGET_DEFINES := __KERNEL__
TARGET_CFLAGS  := -nostdinc -ffreestanding -Wno-unused-function
TARGET_LDFLAGS := -T kernel/kernel.ld

TARGET_LDLIBS := \
    libc.a \

# we only want the .text section right now for our hacky kernel image
RAWBIN_FLAGS := --only-section=.text
$(eval $(call make-rawbin,ohwes.sys,${RAWBIN_FLAGS}))
