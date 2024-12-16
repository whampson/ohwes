TARGET := kernel.elf
TARGET_EXE := ohwes.sys

SOURCES := \
    setup.S \
    entry.S \
    chdev.c \
    console.c \
    cpu.c \
    crash.c \
    fs.c \
    i8042.c \
    irq.c \
    list.c \
    main.c \
    mm.c \
    pic.c \
    pool.c \
    print.c \
    ps2kb.c \
    rtc.c \
    serial.c \
    syscall.c \
    task.c \
    timer.c \
    tty.c \
    tty_ldisc.c \
    vga.c \
    sys/open.c \
    test/pool_test.c \
    # test/list_test.c \

TARGET_DEFINES := __KERNEL__
TARGET_CFLAGS  := -nostdinc -ffreestanding
TARGET_CFLAGS  += -Wno-unused-function -Wno-multichar
TARGET_LDSCRIPT := kernel.ld

TARGET_LDLIBS := \
    libc.a \

# we only want the .text section right now for our hacky kernel image
$(eval $(call make-rawbin,ohwes.sys,-R .eh_frame))
