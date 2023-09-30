TARGET := libkernel.a

SOURCES := \
    entry.S \
    interrupt.S \
    console.c \
    debug.c \
    handler.c \
    init.c \
    irq.c \

TGT_LDLIBS := $(BUILD_DIR)/lib/libcrt.a
TGT_INCDIRS := src/kernel/include
TGT_LDFLAGS := -TText
