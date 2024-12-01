TARGET := kernel.elf
TARGET_EXE := ohwes.sys

SOURCES := \
    setup.S \
    entry.S \
    console.c \
    cpu.c \
    crash.c \
    i8042.c \
    irq.c \
    main.c \
    mm.c \
    pic.c \
    print.c \
    ps2kb.c \
    rtc.c \
    serial.c \
    syscall.c \
    task.c \
    timer.c \
    vga.c \
    sys/open.c \

TARGET_DEFINES := __KERNEL__
TARGET_CFLAGS  := -nostdinc -ffreestanding -Wno-unused-function
TARGET_LDSCRIPT := kernel.ld

TARGET_LDLIBS := \
    libc.a \

# we only want the .text section right now for our hacky kernel image
$(eval $(call make-rawbin,ohwes.sys))
