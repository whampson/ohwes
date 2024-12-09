# architecture
ARCH        := pentium

# includes
INCLUDES    := include

# OS modules
MODULES := \
    boot \
    kernel \
    lib \

ifeq "${TEST_BUILD}" "1"
  MODULES += test
endif

SUBMAKEFILES := $(addsuffix /Module.mk,${MODULES})

# use cross-compiler toolchain
PREFIX     := i686-elf-
AR         := ${PREFIX}ar
AS         := ${PREFIX}gcc
CC         := ${PREFIX}gcc
LD         := ${PREFIX}gcc
OBJCOPY    := ${PREFIX}objcopy

# flags, etc.
ARFLAGS    := -rcsv
CFLAGS     += -std=c11 -include compiler.h -march=${ARCH}
LDFLAGS    += -nostdlib -lgcc
