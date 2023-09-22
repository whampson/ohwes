# =============================================================================
# Copyright (C) 2023 Wes Hampson. All Rights Reserved.
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
#         File: Makefile
#      Created: March 12, 2023
#       Author: Wes Hampson
# =============================================================================

# Modules to build
export MODULES := \
    src/crt \
    src/boot \
    src/kernel \

# Enable debug build
export DEBUG      := 1
export DEBUGFLAGS := -DDEBUG -g -Og

# Important directories
export BIN     := bin
export OBJ     := obj
export SRC     := src
export SCRIPTS := scripts
export MODULE   = $(patsubst %/$(_MODFILE),%,$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))

export _IMG     := $(BIN)/ohwes.img
export _BOOTBIN := $(BIN)/boot.bin

# Build tools
export PREFIX  := i686-elf-
export AR      := $(PREFIX)ar
export AS      := $(PREFIX)gcc
export CC      := $(PREFIX)gcc
export LD      := $(PREFIX)gcc
export OBJCOPY := $(PREFIX)objcopy
export MKDIR   := mkdir -p
export MV      := mv -f
export RM      := rm -f

export INCLUDES      := src/include
export INCLUDE_FILES := compiler.h

# Compiler, assembler, linker flags
export _ASFLAGS       := -Wall -Werror -D__ASSEMBLER__
export _CFLAGS        := -Wall -Werror -nostdinc -ffreestanding -std=c99
export _LDFLAGS       := -nostdlib

ifeq ($(DEBUG),1)
  _ASFLAGS += $(DEBUGFLAGS)
  _CFLAGS  += $(DEBUGFLAGS)
  _LDFLAGS += $(DEBUGFLAGS)
endif

_MODFILE := Module.mk
_OBJS    :=
_LIBS    :=
_BINS    :=
_INCS     = $(INCLUDES)
_DEPS     = $(_OBJS:.o=.d)
_DIRS     = $(call uniq,$(dir $(_OBJS) $(_LIBS) $(_BINS)))

# =================================================================================================

# $(call make-sys, sys-path, elf-path)
define make-sys
  _BINS += $(addprefix $(BIN)/,$1)
  $(addprefix $(BIN)/,$1):: $(addprefix $(BIN)/,$2)
	$(OBJCOPY) -Obinary $$< $$@
endef

# $(call make-exe, exe-path, src-list, [link-libs], [cflags], [asflags], [ldflags])
define make-exe
  _BINS += $(addprefix $(BIN)/,$1)
  $(foreach _src,$(addprefix $(MODULE)/,$(filter %.S,$2)),$(eval $(call make-obj-asm,$(call get-objpath-asm,$(_src)),$(_src),$5)))
  $(foreach _src,$(addprefix $(MODULE)/,$(filter %.c,$2)),$(eval $(call make-obj-c,$(call get-objpath-c,$(_src)),$(_src),$4)))
  $(addprefix $(OBJ)/,$1):: $(call get-objpath-asm,$(addprefix $(MODULE)/,$(filter %.S,$2))) $(call get-objpath-c,$(addprefix $(MODULE)/,$(filter %.c,$2)))
	$(AR) rcsv $$@ $$^
  $(addprefix $(BIN)/,$1):: $(call get-objpath-asm,$(addprefix $(MODULE)/,$(filter %.S,$2))) $(call get-objpath-c,$(addprefix $(MODULE)/,$(filter %.c,$2))) $3
	$(LD) -o $$@ $6 $(_LDFLAGS) $$^
endef

# $(call make-lib, lib-path, src-list, [cflags], [asflags])
define make-lib
  _LIBS += $(addprefix $(OBJ)/,$1)
  $(foreach _src,$(addprefix $(MODULE)/,$(filter %.S,$2)),$(eval $(call make-obj-asm,$(call get-objpath-asm,$(_src)),$(_src),$4)))
  $(foreach _src,$(addprefix $(MODULE)/,$(filter %.c,$2)),$(eval $(call make-obj-c,$(call get-objpath-c,$(_src)),$(_src),$3)))
  $(addprefix $(OBJ)/,$1):: $(call get-objpath-asm,$(addprefix $(MODULE)/,$(filter %.S,$2))) $(call get-objpath-c,$(addprefix $(MODULE)/,$(filter %.c,$2)))
	$(AR) rcsv $$@ $$^
endef

# $(call make-obj-asm, obj-path, src-list, [asflags])
define make-obj-asm
  _OBJS += $1
  $1: $2
	$(AS) -o $$@ $(_ASFLAGS) $3 \
	$(addprefix -I,$(INCLUDES)) \
	-c -MD -MF $$(@:.o=.d) $$<
endef

# $(call make-obj-c, obj-path, src-list, [cflags])
define make-obj-c
  _OBJS += $1
  $1: $2
	$(CC) -o $$@ $(_CFLAGS) $3 \
	$(addprefix -I,$(INCLUDES)) \
	$(addprefix -include , $(INCLUDE_FILES)) \
	-c -MD -MF $$(@:.o=.d) $$<
endef

# $(call get-objpath-asm, src-path)
define get-objpath-asm
  $(subst $(SRC)/,$(OBJ)/,$(subst .S,.o,$(filter %.S,$1)))
endef

# $(call get-objpath-c, src-path)
define get-objpath-c
  $(subst $(SRC)/,$(OBJ)/,$(subst .c,.o,$(filter %.c,$1)))
endef


# $(call uniq, word-list)
#     https://stackoverflow.com/a/16151140
define uniq
  $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))
endef

# =================================================================================================

.PHONY: all floppy img
.PHONY: dirs clean nuke
.PHONY: run-qemu run-qemu-dbg
.PHONY: run-bochs
.PHONY: debug-make

.SECONDARY: $(_OBJS)
.INTERMEDIATE: $(_DEPS)

all:

vpath %.h $(_INCS)
include $(addsuffix /$(_MODFILE), $(MODULES))

# -------------------------------------------------------------------------------------------------

all: $(_DIRS) $(_BINS) $(_LIBS)
img: $(_DIRS) $(_IMG) $(_BINS) $(_LIBS)
dirs: $(_DIRS)

$(_DIRS):
	$(MKDIR) $(call uniq,$(dir $(_OBJS) $(_LIBS) $(_BINS)))


$(_IMG): $(_BOOTBIN)
	dd if=/dev/zero of=$(_IMG) bs=512 count=2880
	dd if=$(_BOOTBIN) of=$(_IMG) bs=512 conv=notrunc
# TODO: add ohwes.sys

floppy: img
	dd if=$(_BOOTBIN) of=/dev/fd0 bs=512 conv=notrunc

clean:
	$(RM) $(_IMG) $(_BINS) $(_LIBS) $(_OBJS) $(_DEPS)

nuke:
	$(RM) -r $(BIN) $(OBJ)

# -------------------------------------------------------------------------------------------------

run-qemu: img
	$(SCRIPTS)/run.sh qemu $(BIN)/ohwes.img

run-qemu-dbg:
	$(SCRIPTS)/run.sh qemu $(BIN)/ohwes.img debug

run-bochs: img
# TODO: try to make the bochs script less platform and directory dependent...
	$(SCRIPTS)/run.sh bochs $(SCRIPTS)/bochsrc.bxrc

# -------------------------------------------------------------------------------------------------

# Include .d files
-include $(_DEPS)
