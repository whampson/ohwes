TARGET         := lib/kernel/mm.a
TARGET_DEFINES := __KERNEL__
TARGET_CFLAGS  := -Wno-multichar -Wno-unused-function

SOURCES := \
    mm.c \
    pool.c \

