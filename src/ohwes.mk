# use cross-compiler toolchain
PREFIX     := i686-elf-
AR         := $(PREFIX)ar
AS         := $(PREFIX)gcc
CC         := $(PREFIX)gcc
LD         := $(PREFIX)gcc
OBJCOPY    := $(PREFIX)objcopy

# flags, etc.
ARFLAGS    := -rcsv
ASFLAGS    := -Wall -Werror -D__ASSEMBLER__
CFLAGS     := -Wall -Werror -nostdinc -ffreestanding -std=c99
LDFLAGS    := -nostdlib

ifeq "${DEBUG}" "1"
  ASFLAGS += ${DEBUGFLAGS}
  CFLAGS += ${DEBUGFLAGS}
endif

# include dirs, relative to the project root for some reason
INCDIRS    := src/include

# OS modules
SUBMAKEFILES := \
    crt/Module.mk \
    kernel/Module.mk \
# boot/Module.mk
