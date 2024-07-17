TARGET := test.elf

SOURCES := \
    test.c \
    queue_tests.c \
    printf_tests.c \

TARGET_LDLIBS := \
    lib.a \

DEFINES := __USER_MODE__
TARGET_LDFLAGS := -T test/test.ld

$(eval $(call make-rawbin,test.exe))
