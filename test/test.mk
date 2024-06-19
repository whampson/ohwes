TARGET := elf/test.elf
TARGETEXE := test/test.exe

SOURCES := \
    test.c \
    char_queue_tests.c \
    printf_tests.c \

LINKLIBS := \
    lib/libc.a \
    lib/libgcc.a \
    lib/libos.a \

TGT_ASFLAGS := -D__USER_MODE__
TGT_CFLAGS  := -D__USER_MODE__
TGT_LDFLAGS := -T test/test.ld

$(eval $(call add-linklibs,${LINKLIBS}))
TGT_POSTMAKE := $(call make-exe,${TARGETEXE},${OBJCOPYFLAGS})
