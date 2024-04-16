# includes
INCDIRS    := include

# OS modules
SUBMAKEFILES := \
    lib/libc/libc.mk \
    lib/libgcc/libgcc.mk \
    boot/x86_boot.mk \
    kernel/kernel.mk \
    init/init.mk \

# use cross-compiler toolchain
PREFIX     := i686-elf-
AR         := $(PREFIX)ar
AS         := $(PREFIX)gcc
CC         := $(PREFIX)gcc
LD         := $(PREFIX)gcc
OBJCOPY    := $(PREFIX)objcopy

# flags, etc.
ARFLAGS    := -rcsv
ASFLAGS    := -Wall -Werror
CFLAGS     := -Wall -Werror -nostdinc -ffreestanding -std=c11
CFLAGS     += -include lib/libgcc/gcc.h    # automatically include GCC stuff
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

# Add libraries to link against for the current module.
#   1 - list of objects to link against, relative to TARGET_DIR
define add-linklibs
  $(eval TGT_PREREQS += $1)
  $(eval TGT_LDLIBS  += $(addprefix ${TARGET_DIR}/,$1))
endef
