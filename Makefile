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
export SCRIPTS  := scripts
export SRC_ROOT := src
export OBJ_ROOT := obj
export BIN_ROOT := bin
export MODULE   = $(subst $(SRC_ROOT)/,,$(SRC))
export SRC      = $(patsubst %/$(_MODFILE),%,$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))
export OBJ      = $(subst $(SRC_ROOT)/,$(OBJ_ROOT)/,$(SRC))
export BIN      = $(BIN_ROOT)

export _IMG     := $(BIN_ROOT)/ohwes.img
export _BOOTBIN := $(BIN_ROOT)/boot.bin

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
_BINS    :=
_INCS     = $(INCLUDES)
_DEPS     = $(subst .o,.d,$(filter %.o,$(_OBJS)))
_DIRS     = $(call uniq,$(dir $(_OBJS) $(_BINS)))

ifndef OHWES_ENVIRONMENT_SET
  $(error "Environment not set! Please source $(SCRIPTS)/env.sh.")
endif

# =================================================================================================

define BINPLACE
  _BINS += $(addprefix $(BIN)/,$1)
  $(addprefix $(BIN)/,$1):: $2
	cp $$< $$@
endef

define STRIP
  _OBJS += $(addprefix $(OBJ)/,$1.bin)
  $(addprefix $(OBJ)/,$1.bin):: $(addprefix $(OBJ)/,$1.elf)
	$(OBJCOPY) -Obinary $$< $$@
endef

# $(call link, elf-path, link-libs, [ldflags])
define LINK
  _OBJS += $(addprefix $(OBJ)/,$1.elf)
  $(addprefix $(OBJ)/,$1.elf):: $(addprefix $(OBJ)/,$1.lib) $2
	$(LD) -o $$@ $3 $(_LDFLAGS) $$^
endef

# $(call compile, lib-path, src-list, [cflags], [asflags])
define COMPILE
  _OBJS += $(addprefix $(OBJ)/,$1.lib)
  $(foreach _src,$(addprefix $(SRC)/,$(filter %.S,$2)),$(eval $(call make-obj-asm,$(call get-objpath-asm,$(_src)),$(_src),$4)))
  $(foreach _src,$(addprefix $(SRC)/,$(filter %.c,$2)),$(eval $(call make-obj-c,$(call get-objpath-c,$(_src)),$(_src),$3)))
  $(addprefix $(OBJ)/,$1.lib):: $(call get-objpath-asm,$(addprefix $(SRC)/,$(filter %.S,$2))) $(call get-objpath-c,$(addprefix $(SRC)/,$(filter %.c,$2)))
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
  $(subst $(SRC_ROOT)/,$(OBJ_ROOT)/,$(subst .S,.o,$(filter %.S,$1)))
endef

# $(call get-objpath-c, src-path)
define get-objpath-c
  $(subst $(SRC_ROOT)/,$(OBJ_ROOT)/,$(subst .c,.o,$(filter %.c,$1)))
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
include $(addsuffix /$(_MODFILE),$(MODULES))

# -------------------------------------------------------------------------------------------------

all: $(_BINS)
img: $(_IMG) $(_BINS)

# $(info $(MKDIR) $(_DIRS))
$(shell $(MKDIR) $(_DIRS))

$(_IMG): $(_BOOTBIN)
	dd if=/dev/zero of=$(_IMG) bs=512 count=2880
	dd if=$(_BOOTBIN) of=$(_IMG) bs=512 conv=notrunc
# TODO: add ohwes.sys

floppy: img
	dd if=$(_BOOTBIN) of=/dev/fd0 bs=512 conv=notrunc

clean:
	$(RM) $(_IMG) $(_BINS) $(_OBJS) $(_DEPS)

nuke:
	$(RM) -r $(BIN_ROOT) $(OBJ_ROOT)

# -------------------------------------------------------------------------------------------------

run-qemu: img
	$(SCRIPTS)/run.sh qemu $(BIN_ROOT)/ohwes.img

run-qemu-dbg:
	$(SCRIPTS)/run.sh qemu $(BIN_ROOT)/ohwes.img debug

run-bochs: img
# TODO: try to make the bochs script less platform and directory dependent...
	$(SCRIPTS)/run.sh bochs $(SCRIPTS)/bochsrc.bxrc

# -------------------------------------------------------------------------------------------------

# Include .d files
-include $(_DEPS)
