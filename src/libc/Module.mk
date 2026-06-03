TARGET  := lib/libc.a
SOURCES := \
    ctype.c \
    errno.c \
    linkage.c \
    printf.c \
    stdio.c \
    string.c \

ifeq (${TEST_LIBC}, 1)
  SOURCES += \
      test/libc_tests.c \
      test/framework.c \
      test/test_ctype.c \
      test/test_printf.c \
      test/test_ring.c \
      test/test_stdarg.c \
      test/test_stdlib_math.c \
      test/test_stdtypes.c \
      test/test_string.c \
      test/test_strtol.c
endif
