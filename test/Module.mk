TARGET := test.elf
TARGETEXE := test.exe

SOURCES := \
    test.c \
    queue_tests.c \
    printf_tests.c \

LINKLIBS := \
    lib.a \

TGT_ASFLAGS := -D__USER_MODE__
TGT_CFLAGS  := -D__USER_MODE__
TGT_LDFLAGS := -T test/test.ld

$(eval $(call add-linklibs,${LINKLIBS}))
TGT_POSTMAKE := $(call make-exe,${TARGETEXE},${OBJCOPYFLAGS})
