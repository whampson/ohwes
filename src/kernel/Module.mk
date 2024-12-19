TARGET := kernel.elf
TARGET_EXE := ohwes.sys

SOURCES := \
    crash.c \
    irq.c \
    list.c \
    print.c \
    ring.c \
    task.c \

TARGET_DEFINES := __KERNEL__
TARGET_CFLAGS  := -Wno-unused-function -Wno-multichar
TARGET_LDSCRIPT:= kernel.ld

MODULES := \
    drivers/char \
    init \
    fs \
    mm \

SUBMAKEFILES := $(addsuffix /Module.mk,${MODULES})

TARGET_LDLIBS := \
    lib/kernel/${ARCH}.a \
    lib/kernel/drivers/char.a \
    lib/kernel/init.a \
    lib/kernel/fs.a \
    lib/kernel/mm.a \
    lib/libc.a \

TARGET_LDFLAGS := \
    -Wl,--start-group \
    $(addprefix ${TARGET_DIR}/,${TARGET_LDLIBS}) \
    -Wl,--end-group

RAWBIN_ARGS := -R .eh_frame	# no .eh_frame gobbledygook
$(eval $(call make-rawbin,ohwes.sys,${RAWBIN_ARGS}))
