# include dirs, relative to the project root for some reason
INCDIRS    := src/include

# OS modules
SUBMAKEFILES := \
    crt/Module.mk \
    kernel/Module.mk \
    boot/Module.mk

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

# Create a raw binary executable, no symbols or anything
#   1 - input ELF
#   2 - output file
define raw-bin
	@mkdir -p $(dir $2)
	${OBJCOPY} -Obinary $1 $2
endef

# Create a system (.sys) file from the current TARGET file.
#   1 - system file target path
define make-sys
  $(call raw-bin,${TARGET_DIR}/${TARGET},${TARGET_DIR}/$1)
  $(eval TGT_POSTCLEAN += ${RM} ${TARGET_DIR}/$1)
endef
