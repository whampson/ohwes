TARGET := elf/init.elf
TARGETEXE := init/init.exe

SOURCES := \
    _crt.c \
    entry.S \
    init.c \
    rtc_test.c \

TGT_CFLAGS := -D__USER_MODE__
TGT_LDFLAGS := -T init/init.ld

$(eval $(call add-linklibs,${LINKLIBS}))
TGT_POSTMAKE := $(call make-exe,${TARGETEXE},${OBJCOPYFLAGS})
