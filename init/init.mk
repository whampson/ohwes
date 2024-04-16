TARGET := lib/init.a

SOURCES := \
    _crt.c \
    init.c \
    rtc_test.c \

TGT_CFLAGS := -D__USER_MODE__
