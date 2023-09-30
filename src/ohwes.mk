
PREFIX  := i686-elf-
AR      := $(PREFIX)ar
AS      := $(PREFIX)gcc
CC      := $(PREFIX)gcc
LD      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy

ASFLAGS := -Wall -Werror -D__ASSEMBLER__
CFLAGS  := -Wall -Werror -nostdinc -ffreestanding -std=c99
LDFLAGS := -nostdlib

# relative to the project root for some reason
INCDIRS := src/include

SUBMAKEFILES := \
    crt/Module.mk \
#    kernel/Module.mk \

