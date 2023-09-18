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
export MODULES      := boot kernel sdk/libc
# TODO: search tree for modules?

# Important directories
export SCRIPTS      := scripts
export BIN_ROOT     := bin
export OBJ_ROOT     := obj
export LIB_ROOT     := obj
export MOD_ROOT      = $(patsubst %/$(_MODFILE),%,$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))

export IMG          := $(BIN_ROOT)/ohwes.img
export BOOTBIN      := $(BIN_ROOT)/boot.bin

# Build tools
BINUTILS_PREFIX     := i686-elf-
export AR           := $(BINUTILS_PREFIX)ar
export AS           := $(BINUTILS_PREFIX)gcc
export CC           := $(BINUTILS_PREFIX)gcc
export LD           := $(BINUTILS_PREFIX)gcc
export OBJCOPY      := $(BINUTILS_PREFIX)objcopy
export MKDIR        := mkdir -p
export MV           := mv -f
export RM           := rm -f

# Compiler, assembler, linker flags
export ASFLAGS      := -D__ASSEMBLER__
export CFLAGS       := -nostdinc -ffreestanding -std=c99
export LDFLAGS      := -nostdlib

# Default defines
export DEFINES      :=

# Default include directories
export INCLUDES     := include

# Default include files
export INCLUDE_FILES:= compiler.h

# Default warnings
export WARNINGS     := all error

# Enable debug build
export DEBUG        := 1

# $(call make-exe exe-name, source-list[, link-libs])
define make-exe
  _TARGETS += $(addprefix $(BIN_ROOT)/,$1)
  _SOURCES += $(addprefix $(MOD_ROOT)/,$2)
  $(addprefix $(BIN_ROOT)/,$1):: $(call get-asm-obj, $(addprefix $(MOD_ROOT)/,$2)) $(call get-c-obj, $(addprefix $(MOD_ROOT)/,$2)) $3
	$(LD) -o $$@ $(LDFLAGS) $$^
endef

# $(call make-lib lib-name, source-list)
define make-lib
  _LIBRARIES += $(addprefix $(LIB_ROOT)/$(MOD_ROOT)/,$1)
  _SOURCES += $(addprefix $(MOD_ROOT)/,$2)
  $(addprefix $(LIB_ROOT)/$(MOD_ROOT)/,$1):: $(call get-asm-obj, $(addprefix $(MOD_ROOT)/,$2)) $(call get-c-obj, $(addprefix $(MOD_ROOT)/,$2))
	$(AR) rcsv $$@ $$^
endef

# =================================================================================================

_MODFILE        := Module.mk
_SOURCES        :=
_OBJECTS        :=
_TARGETS        :=
_LIBRARIES      :=

_MODULES         = $(MODULES)
_INCLUDES        = $(INCLUDES)
_DEPENDS         = $(_OBJECTS:.o=.d)
_DIRS            = $(call uniq, $(dir $(_OBJECTS) $(_TARGETS) $(_LIBRARIES)))
_IMG             = $(IMG)

get-c-src        = $(filter %.c,$1)
get-asm-src      = $(filter %.S,$1)

get-c-obj        = $(addprefix $(OBJ_ROOT)/,$(subst .c,.o,$(call get-c-src,$1)))
get-asm-obj      = $(addprefix $(OBJ_ROOT)/,$(subst .S,.o,$(call get-asm-src,$1)))

ifdef DEBUG
  DEFINES += DEBUG
  CFLAGS += -g -Og
  LDFLAGS += -g -Og
  ASLAGS += -g -Og
endif

# uniq - https://stackoverflow.com/a/16151140
uniq = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))

# $(call make-obj obj-path, source-path)
define make-obj
  _OBJECTS += $1
  $1: $2
	$(CC) -c $(CFLAGS) \
	$(addprefix -W,$(WARNINGS)) \
	$(addprefix -D,$(DEFINES)) \
	$(addprefix -I,$(INCLUDES)) \
	$(addprefix -include , $(INCLUDE_FILES)) \
	-MD -MF $$(@:.o=.d) -o $$@ $$<
endef

# $(call make-obj obj-path, source-path)
define make-asm-obj
  _OBJECTS += $1
  $1: $2
	$(AS) -c $(ASFLAGS) \
	$(addprefix -W,$(WARNINGS)) \
	$(addprefix -D,$(DEFINES)) \
	$(addprefix -I,$(INCLUDES)) \
	-MD -MF $$(@:.o=.d) -o $$@ $$<
endef

# =================================================================================================

.PHONY: all floppy img
.PHONY: dirs clean nuke
.PHONY: run-qemu run-bochs
.PHONY: debug-make

.SECONDARY: $(_OBJECTS)
.INTERMEDIATE: $(_DEPENDS)

all:

vpath %.h $(_INCLUDES)
include $(addsuffix /$(_MODFILE), $(_MODULES))

# -------------------------------------------------------------------------------------------------

all: dirs img $(_TARGETS) $(_LIBRARIES)

dirs: $(_DIRS)

$(_DIRS):
	$(MKDIR) $(_DIRS)

img: dirs $(_IMG) $(_TARGETS) $(_LIBRARIES)

$(_IMG): $(BOOTBIN)
	dd if=/dev/zero of=$(_IMG) bs=512 count=2880
	dd if=$(BOOTBIN) of=$(_IMG) bs=512 conv=notrunc

floppy: img
	dd if=$(BOOTBIN) of=/dev/fd0 bs=512 conv=notrunc

clean:
	$(RM) $(_IMG) $(_TARGETS) $(_LIBRARIES) $(_OBJECTS) $(_DEPENDS)

nuke:
	$(RM) -r $(BIN_ROOT) $(OBJ_ROOT) $(LIB_ROOT)

# -------------------------------------------------------------------------------------------------

run-qemu: img
	$(SCRIPTS)/run.sh qemu $(BIN_ROOT)/ohwes.img

run-bochs: img
	$(SCRIPTS)/run.sh bochs $(SCRIPTS)/bochsrc.bxrc

# -------------------------------------------------------------------------------------------------

debug-make:
	@echo 'MODULES       = $(MODULES)'
	@echo 'DEFINES       = $(DEFINES)'
	@echo 'INCLUDES      = $(INCLUDES)'
	@echo 'WARNINGS      = $(WARNINGS)'
	@echo 'DEBUG         = $(DEBUG)'
	@echo 'ASFLAGS       = $(ASFLAGS)'
	@echo 'CFLAGS        = $(CFLAGS)'
	@echo 'LDFLAGS       = $(LDFLAGS)'
	@echo 'AR            = $(AR)'
	@echo 'AS            = $(AS)'
	@echo 'CC            = $(CC)'
	@echo 'LD            = $(LD)'
	@echo 'OBJCOPY       = $(OBJCOPY)'
	@echo 'MKDIR         = $(MKDIR)'
	@echo 'MV            = $(MV)'
	@echo 'RM            = $(RM)'
	@echo 'SCRIPTS       = $(SCRIPTS)'
	@echo 'BIN_ROOT      = $(BIN_ROOT)'
	@echo 'OBJ_ROOT      = $(OBJ_ROOT)'
	@echo '--------------------------------------------------------------------------------'
	@echo 'MAKEFILE_LIST = $(MAKEFILE_LIST)'
	@echo '_MODULES      = $(_MODULES)'
	@echo '_TARGETS      = $(_TARGETS)'
	@echo '_LIBRARIES    = $(_LIBRARIES)'
	@echo '_SOURCES      = $(_SOURCES)'
	@echo '_INCLUDES     = $(_INCLUDES)'
	@echo '_OBJECTS      = $(_OBJECTS)'
	@echo '_DEPENDS      = $(_DEPENDS)'
	@echo '_DIRS         = $(_DIRS)'

# -------------------------------------------------------------------------------------------------

# Generate object file rules from source list
$(foreach _src, $(call get-c-src, $(_SOURCES)), $(eval $(call make-obj, $(call get-c-obj, $(_src)), $(_src))))
$(foreach _src, $(call get-asm-src, $(_SOURCES)), $(eval $(call make-asm-obj, $(call get-asm-obj, $(_src)), $(_src))))

# Include .d files
-include $(_DEPENDS)
