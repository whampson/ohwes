# target architecture
ARCH        := i386
ifeq "${ARCH}" "i386"
  MARCH     := pentium
endif

# include directory (from repo root)
INCLUDES    := src/include

# OS modules to build
MODULES := \
    ${ARCH}/boot \
    ${ARCH}/kernel \
    kernel \
    libc \

SUBMAKEFILES := $(addsuffix /Module.mk,${MODULES})

# cross-compiler toolchain
PREFIX     := i686-elf-
AR         := ${PREFIX}ar
AS         := ${PREFIX}gcc
CC         := ${PREFIX}gcc
LD         := ${PREFIX}gcc
OBJCOPY    := ${PREFIX}objcopy

# default flags
ARFLAGS    := -rcsv
CFLAGS     += -std=c11 -march=${MARCH} -ffreestanding -nostdinc
CFLAGS     += -include ${ARCH}/compiler.h
LDFLAGS    += -nostdlib -lgcc
