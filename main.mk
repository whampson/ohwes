#==============================================================================
# Copyright (C) 2020-2026 Wes Hampson. All Rights Reserved.
#
# This file is part of the OH-WES Operating System.
# OH-WES is free software; you may redistribute it and/or modify it under the
# terms of the GNU GPLv2. See the LICENSE file in the root of this repository.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# -----------------------------------------------------------------------------
#         File: main.mk
#      Created: September 29, 2023
#      Revised: November 3, 2026
#       Author: Wes Hampson
#
# boilermake system entrypoint for ohwes
# =============================================================================

ifndef PROJ_ROOT
  $(error "PROJ_ROOT not defined! Did you forget to source env.sh?")
endif

# build type defines
export DEBUG            ?= 1
export TEST_BUILD       ?= 0
export TEST_LIBC        ?= 0

# optimization controls
export OPTIMIZE         ?= 1
export OPTIMIZE_DEBUG   ?= 1
export OPTIMIZE_PARAMS  ?=

# required for boilermake
export TARGET_DIR       := ${PROJ_ROOT}/bin
export BUILD_DIR        := ${PROJ_ROOT}/obj

# other important dirs
export SCRIPT_DIR       := ${PROJ_ROOT}/scripts

# default flags
export DEFAULT_ARFLAGS  :=
export DEFAULT_ASFLAGS  := -Wa,--fatal-warnings
export DEFAULT_LDFLAGS  := -Wl,--fatal-warnings
export DEFAULT_CFLAGS   := -Wall -Werror -Wno-unused-function
export DEFAULT_CXXFLAGS := ${DEFAULT_CFLAGS}

# submodules
SUBMODULES := src

# debug build config
ifeq "${DEBUG}" "1"
  DEFINES  += DEBUG
  DEFAULT_ASFLAGS  += -g
  DEFAULT_CFLAGS   += -g
  DEFAULT_CXXFLAGS += -g
  ifeq "${OPTIMIZE_DEBUG}" "1"
    DEFAULT_CFLAGS   += -Og
    DEFAULT_CXXFLAGS += -Og
  endif
else ifneq "${OPTIMIZE}" "0"
  DEFAULT_CFLAGS   += -O${OPTIMIZE} ${OPTIMIZE_PARAMS}
  DEFAULT_CXXFLAGS += -O${OPTIMIZE} ${OPTIMIZE_PARAMS}
endif

# test build config
ifeq "${TEST_BUILD}" "1"
  DEFINES += TEST_BUILD
endif
ifeq "${TEST_LIBC}" "1"
  DEFINES += TEST_LIBC
endif

# -----------------------------------------------------------------------------
# disk image

BOOTSECT    := ${TARGET_DIR}/boot/boot.bin
DISK_IMAGE  := ${TARGET_DIR}/ohwes.img

define CREATE_IMAGE_CMDS
	@mkdir -p $(dir ${DISK_IMAGE})
	fatfs create --force ${DISK_IMAGE} 2880
	dd if=${BOOTSECT} of=${DISK_IMAGE} bs=512 count=1 conv=notrunc
	$(foreach file,${DISK_FILES},\
	    fatfs add ${DISK_IMAGE} ${file} $(notdir ${file});\
	    fatfs attr -s ${DISK_IMAGE} $(notdir ${file});)
	fatfs list -Aa ${DISK_IMAGE}
endef

# -----------------------------------------------------------------------------
# floppy disk stuff

ifeq "${HOST_SYSTEM}" "msys2"
  DISK_MOUNT ?= /a
else ifeq "${HOST_SYSTEM}" "macos"
  DISK_MOUNT ?= /Volumes/OHWES
else ifeq "${HOST_SYSTEM}" "linux"
  DISK_MOUNT ?= /mnt/floppy
endif

DISK_DEVICE := /dev/fd0
DISK_FILES  := \
    ${TARGET_DIR}/boot/boot.sys \
    ${TARGET_DIR}/ohwes.sys

define FLOPPY_FORMAT_CMDS
	mkdosfs -s 1 -S 512 ${DISK_DEVICE}
endef
define FLOPPY_COPY_CMDS
	dd if=${BOOTSECT} of=${DISK_DEVICE} bs=512 count=1 conv=notrunc
	$(foreach file,${DISK_FILES},cp ${file} ${DISK_MOUNT}/;)
	ls -l ${DISK_MOUNT}
endef


# -----------------------------------------------------------------------------
# Build Targets
# -----------------------------------------------------------------------------

.PHONY: all ohwes tools

# `all` is provided by the boilermake engine (Makefile) and builds every
# registered target; `ohwes` is a convenient alias for it.
ohwes: all

# supplemental tools
# note: we use -C to call the Makefile directly so we can override the
# toolchain; `tools` is built using the host's native toolchain, so we can't
# include it as a submodule, or else it'll "mix" with the `ohwes` toolchain
tools:
	@${MAKE} -C tools

# -----------------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------------

.PHONY: clean clean-tools clean-all nuke nuke-obj

clean-all: clean clean-tools

# `clean` is provided by boilermake's per-target clean;
# we can extend it here to also remove the disk image
clean:
	${RM} ${DISK_IMAGE}

# clean tools build artifacts
clean-tools:
	${MAKE} -C tools clean

# destroy 'em all!
nuke:
	${RM} -r ${TARGET_DIR} ${BUILD_DIR}

# remove object tree only
nuke-obj:
	${RM} -r ${BUILD_DIR}

# remove build targets only
nuke-bin:
	${RM} -r ${TARGET_DIR}

# -----------------------------------------------------------------------------
# Disk Image
# -----------------------------------------------------------------------------

.PHONY: img floppy format-floppy

# make floppy disk image file
img: tools ohwes
	${CREATE_IMAGE_CMDS}

# write OS binaries to floppy disk (using host's FAT12 driver);
# boot sector is overwritten with OH-WES bootloader
floppy: ohwes
	${FLOPPY_COPY_CMDS}

# format floppy disk to FAT12 (WARNING: DRETROYS ALL DATA)
format-floppy: ohwes
	${FLOPPY_FORMAT_CMDS}

# -----------------------------------------------------------------------------
# Run & Debug
# -----------------------------------------------------------------------------

.PHONY: run run-bochs debug debug-boot debug-setup debug-nobreak debug-detach

RUN        := ${SCRIPT_DIR}/run.sh
RUNQEMU    := ${RUN} --img ${DISK_IMAGE}
RUNFLAGS   :=

run: img
	${RUNQEMU} ${RUNFLAGS}

run-bochs: img
	${RUN} --bochs $(if ${BOCHSRC},--bochsrc ${BOCHSRC}) ${RUNFLAGS}

debug: img
	${RUNQEMU} --debug kernel ${RUNFLAGS}

debug-boot: img
	${RUNQEMU} --debug boot ${RUNFLAGS}

debug-setup: img
	${RUNQEMU} --debug setup ${RUNFLAGS}

debug-nobreak: img
	${RUNQEMU} --debug kernel --no-break ${RUNFLAGS}

debug-detach: img
	${RUNQEMU} --debug kernel --no-gdb ${RUNFLAGS}

# -----------------------------------------------------------------------------
# Help
# -----------------------------------------------------------------------------

.PHONY: help

help:
	@printf '%s\n' \
	  'OH-WES Build System' \
	  '' \
	  'Usage:' \
	  '  make [target] [VAR=value ...]' \
	  '' \
	  'Build Targets:' \
	  '  all, ohwes       build OS binaries' \
	  '  tools            build host tools' \
	  '  img              create bootable disk image of OS binaries' \
	  '  floppy           write boot sector and OS binaries to physical floppy disk' \
	  '  format-floppy    format physical floppy disk as FAT12 (DESTROYS ALL DATA)' \
	  '' \
	  'Clean Targets:' \
	  '  clean            delete OS binaries and disk image' \
	  '  clean-tools      delete host tools binaries' \
	  '  clean-all        clean + clean-tools' \
	  '  nuke             delete bin/ and obj/ (squeaky clean)' \
	  '  nuke-obj         delete obj/ only' \
	  '  nuke-bin         delete bin/ only' \
	  '' \
	  'Run Targets:' \
	  '  run              boot OH-WES in QEMU' \
	  '  run-bochs        boot OH-WES in Bochs' \
	  '  debug            boot kernel in QEMU under GDB' \
	  '  debug-boot       debug boot stage' \
	  '  debug-setup      debug early kernel setup stage' \
	  '  debug-nobreak    debug without initial breakpoint' \
	  '  debug-detach     debug without the GDB stub attached' \
	  '' \
	  'Useful Variables:' \
	  '  V=1              verbose build output' \
	  '  DEBUG=1          enable debug build' \
	  '  TEST_BUILD=1     build test legacy harness' \
	  '  TEST_LIBC=1      build libc test suite' \
	  '  OPTIMIZE=n       set optimization level (0-3)' \
	  '  DISK_MOUNT=...   override the floppy mount point' \
	  '  RUNFLAGS=...     extra flags forwarded to run.sh' \
	  '' \
	  'Toolchain:' \
	  '  The cross-compiler is selected by the TOOLCHAIN environment variable.' \
	  '  (current is ${TOOLCHAIN}).' \
	  ''

