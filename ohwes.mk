# includes
INCDIRS    := include

# OS modules
SUBMAKEFILES := \
    lib/libc/libc.mk \
    lib/libgcc/libgcc.mk \
    lib/libos/libos.mk \
    boot/x86_boot.mk \
    kernel/kernel.mk \
    init/init.mk \

ifeq "${TEST_BUILD}" "1"
  SUBMAKEFILES += test/test.mk
endif

# use cross-compiler toolchain
PREFIX     := i686-elf-
AR         := $(PREFIX)ar
AS         := $(PREFIX)gcc
CC         := $(PREFIX)gcc
LD         := $(PREFIX)gcc
OBJCOPY    := $(PREFIX)objcopy

# flags, etc.
ARFLAGS    := -rcsv
ASFLAGS    := ${GLOBAL_FLAGS}
CFLAGS     := ${GLOBAL_FLAGS} -nostdinc -ffreestanding -std=c11
CFLAGS     += -include lib/libgcc/gcc.h    # automatically include GCC stuff
LDFLAGS    := ${GLOBAL_FLAGS} -nostdlib

ifeq "${DEBUG}" "1"
  ASFLAGS += ${DEBUGFLAGS}
  CFLAGS += ${DEBUGFLAGS}
else
  CFLAGS += -O1
endif

# Create a raw binary executable, no symbols or anything
#   1 - input ELF
#   2 - output file
#   3 - additional objcopy arguments
define raw-bin
	@mkdir -p $(dir $2)
	${OBJCOPY} -Obinary $3 $1 $2
endef

# Create an executable (.exe) file from the current TARGET file.
#   1 - system file target path
#   2 - additional objcopy arguments
define make-exe
  $(call raw-bin,${TARGET_DIR}/${TARGET},${TARGET_DIR}/$1,$2)
  $(eval TGT_POSTCLEAN += ${RM} ${TARGET_DIR}/$1)
endef

# Add libraries to link against for the current module.
#   1 - list of objects to link against, relative to TARGET_DIR
define add-linklibs
  $(eval TGT_PREREQS += $1)
  $(eval TGT_LDLIBS  += $(addprefix ${TARGET_DIR}/,$1))
endef
