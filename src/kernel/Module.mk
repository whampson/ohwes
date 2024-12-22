TARGET := kernel.elf
TARGET_EXE := ohwes.sys

SOURCES := \
    crash.c \
    irq.c \
    list.c \
    print.c \
    ring.c \
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

TARGET_DEFINES := __KERNEL__
TARGET_CFLAGS  := -Wno-unused-function -Wno-multichar
TARGET_LDSCRIPT:= kernel.ld

MODULES := \
    chdev \
    init \
    fs \
    mm \

SUBMAKEFILES := $(addsuffix /Module.mk,${MODULES})

TARGET_LDLIBS := \
    lib/libc.a \
    lib/kernel/${ARCH}.a \
    $(addsuffix .a,$(addprefix lib/kernel/,${MODULES}))

# allow the above libraries to be searched multiple times for symbols
TARGET_LDFLAGS      := -Wl,--start-group
TARGET_LDFLAGS_POST := -Wl,--end-group

RAWBIN_ARGS := -R .eh_frame	# no .eh_frame gobbledygook
$(eval $(call make-rawbin,ohwes.sys,${RAWBIN_ARGS}))
