TARGET := kernel.elf
TARGET_EXE := ohwes.sys

MODULES := char

TARGET_CFLAGS  := -Wno-unused-function -Wno-multichar
TARGET_DEFINES := __KERNEL__
TARGET_LDSCRIPT:= ../${ARCH}/kernel/kernel.ld

# allow the above libraries to be searched multiple times for symbols
TARGET_LDFLAGS      := -Wl,--start-group
TARGET_LDFLAGS_POST := -Wl,--end-group

TARGET_LDLIBS  := \
    lib/libc.a \
    lib/kernel/${ARCH}.a \
    $(addsuffix .a,$(addprefix lib/kernel/,${MODULES})) \
    usr/shell.a	 # TODO: temp until we can load programs :D

SOURCES := \
    console.c \
    fs.c \
    io.c \
    irq.c \
    list.c \
    main.c \
    mm.c \
    open.c \
    pool.c \
    ring.c \
    sys.c \
    task.c \

ifeq "${TEST_BUILD}" "1"
SOURCES += \
    test/test.c \
    test/test_bsf.c \
    test/test_list.c \
    test/test_pool.c \
    test/test_printf.c \
    test/test_ring.c \
    test/test_string.c \

endif


SUBMAKEFILES := $(addsuffix /Module.mk,${MODULES})

$(eval $(call make-rawbin,ohwes.sys))
