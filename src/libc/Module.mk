TARGET  := lib/libc.a
SOURCES := \
    ctype.c \
    errno.c \
    linkage.c \
    printf.c \
    stdio.c \
    string.c \

SOURCES += \
    test/main.c \
    test/framework.c \
    test/test_ctype.c \
    test/test_printf.c \
    test/test_stdarg.c \
    test/test_stdlib_math.c \
    test/test_stdtypes.c \
    test/test_string.c \
    test/test_strtol.c \
