# TODO: search tree for modules?
export MODULES  := \
	boot/x86

export ROOT     := $(CURDIR)
export BIN_ROOT := bin
export OBJ_ROOT := obj
export SCRIPTS  := scripts

export INCLUDES := inc
export DEFINES  :=

BINUTILS_PREFIX := i686-elf-
export AS       := $(BINUTILS_PREFIX)gcc
export CC       := $(BINUTILS_PREFIX)gcc
export LD       := $(BINUTILS_PREFIX)ld
export OBJCOPY  := $(BINUTILS_PREFIX)objcopy
export MKDIR    := mkdir -p
export MV       := mv -f
export RM       := rm -f

export ASFLAGS  := -D__ASSEMBLER__ -g -Wa,-mtune=i386
export CFLAGS   := -g -nostdinc -nostdlib -ffreestanding

export WARNINGS := all error \
	no-trigraphs

# Current module directory.
# Use only in recipies that will be evaulated by a module Makefile.
export MODDIR    = $(patsubst %/$(_MODFILE),%,$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))

# =================================================================================================

_MODFILE        := Module.mk
_SOURCES        :=
_OBJECTS        :=
_TARGETS        :=

_MODULES         = $(MODULES)
_INCLUDES        = $(INCLUDES)
_DEPENDS         = $(_OBJECTS:.o=.d)
_DIRS            = $(call uniq, $(dir $(_OBJECTS) $(_TARGETS)))

get-c-src        = $(filter %.c,$1)
get-asm-src      = $(filter %.S,$1)

get-c-obj        = $(addprefix $(OBJ_ROOT)/,$(subst .c,.o,$(call get-c-src,$1)))
get-asm-obj      = $(addprefix $(OBJ_ROOT)/,$(subst .S,.o,$(call get-asm-src,$1)))

# uniq - https://stackoverflow.com/a/16151140
uniq = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))

define make-exe
  _TARGETS += $(addprefix $(BIN_ROOT)/,$1)
  _SOURCES += $(addprefix $(MODDIR)/,$2)
  $(addprefix $(BIN_ROOT)/,$1):: $(call get-asm-obj, $(addprefix $(MODDIR)/,$2)) $(call get-c-obj, $(addprefix $(MODDIR)/,$2))
	$(LD) $(LDFLAGS) -o $$@ $$^
endef

# $(call make-obj obj-path, source-path)
define make-obj
  _OBJECTS += $1
  $1: $2
	$(CC) $(CFLAGS) $(addprefix -D,$(DEFINES)) $(addprefix -I,$(INCLUDES)) $(addprefix -W,$(WARNINGS)) -c -MD -MF $$(@:.o=.d) -o $$@ $$<
endef

# $(call make-obj obj-path, source-path)
define make-asm-obj
  _OBJECTS += $1
  $1: $2
	$(AS) $(ASFLAGS) $(addprefix -D,$(DEFINES)) $(addprefix -I,$(INCLUDES)) $(addprefix -W,$(WARNINGS)) -c -MD -MF $$(@:.o=.d) -o $$@ $$<
endef

# =================================================================================================

.PHONY: all clean nuke dirs
.PHONY: img floppy
.PHONY: run-qemu run-bochs
.PHONY: debug-make

.SECONDARY: $(_OBJECTS)

all:

vpath %.h $(_INCLUDES)
include $(addsuffix /$(_MODFILE), $(_MODULES)) # TODO: module-specific flags

# -------------------------------------------------------------------------------------------------

all: dirs $(_TARGETS)

clean:
	$(RM) $(_TARGETS) $(_OBJECTS) $(_DEPENDS)

nuke:
	$(RM) -r $(BIN_ROOT) $(OBJ_ROOT)

dirs:
	$(MKDIR) $(_DIRS)

# -------------------------------------------------------------------------------------------------

img: dirs $(_TARGETS)
	dd if=/dev/zero of=$(BIN_ROOT)/ohwes.img bs=512 count=2880
	dd if=$(BIN_ROOT)/boot.bin of=$(BIN_ROOT)/ohwes.img bs=512 conv=notrunc

floppy: img
	dd if=$(BIN_ROOT)/boot.bin of=/dev/fd0 bs=512 conv=notrunc

# -------------------------------------------------------------------------------------------------

run-qemu: img
	$(SCRIPTS)/run.sh qemu $(BIN_ROOT)/ohwes.img

run-bochs: img
	$(SCRIPTS)/run.sh bochs $(SCRIPTS)/bochsrc.bxrc

# -------------------------------------------------------------------------------------------------

debug-make:
	@echo 'MODULES       = $(MODULES)'
	@echo 'INCLUDES      = $(INCLUDES)'
	@echo 'CFLAGS        = $(CFLAGS)'
	@echo 'DEFINES       = $(DEFINES)'
	@echo 'CC            = $(CC)'
	@echo 'MKDIR         = $(MKDIR)'
	@echo 'MV            = $(MV)'
	@echo 'RM            = $(RM)'
	@echo '--------------------------------------------------------------------------------'
	@echo 'ROOT          = $(ROOT)'
	@echo 'BIN_ROOT      = $(BIN_ROOT)'
	@echo 'OBJ_ROOT      = $(OBJ_ROOT)'
	@echo 'MAKEFILE_LIST = $(MAKEFILE_LIST)'
	@echo '_MODULES      = $(_MODULES)'
	@echo '_TARGETS      = $(_TARGETS)'
	@echo '_SOURCES      = $(_SOURCES)'
	@echo '_INCLUDES     = $(_INCLUDES)'
	@echo '_OBJECTS      = $(_OBJECTS)'
	@echo '_DEPENDS      = $(_DEPENDS)'

# -------------------------------------------------------------------------------------------------

# generate object file rules
$(foreach _src, $(call get-c-src, $(_SOURCES)), $(eval $(call make-obj, $(call get-c-obj, $(_src)), $(_src))))
$(foreach _src, $(call get-asm-src, $(_SOURCES)), $(eval $(call make-asm-obj, $(call get-asm-obj, $(_src)), $(_src))))

-include $(_DEPENDS)
