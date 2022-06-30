#===============================================================================
#    File: Rules.mk
# Created: June 27, 2022
#  Author: Wes Hampson
#
# Makefile rules common to all parts of the build.
#===============================================================================

# ------------------------------------------------------------------------------
# Sanity checking
ifndef _OSROOT
  $(error "Environment variable '_OSROOT' not defined! Did you remember to source env.sh?")
endif
ifndef _SRCROOT
  $(error "Environment variable '_SRCROOT' not defined! Did you remember to source env.sh?")
endif
ifndef _BINROOT
  $(error "Environment variable '_BINROOT' not defined! Did you remember to source env.sh?")
endif
ifndef _OBJROOT
  $(error "Environment variable '_OBJROOT' not defined! Did you remember to source env.sh?")
endif
ifndef _SCRIPTS
  $(error "Environment variable '_SCRIPTS' not defined! Did you remember to source env.sh?")
endif

# ------------------------------------------------------------------------------
# Flags

AS_WARN = -Wall -Wextra -Werror -Wpedantic
CC_WARN = -Wall -Wextra -Werror -Wpedantic

AS_DEBUG_SYM = -ggdb
CC_DEBUG_SYM = -ggdb

AS_DEBUG_DEFINE = -DDEBUG
CC_DEBUG_DEFINE = -DDEBUG

# TODO: define these
AS_OPT =
CC_OPT =

export MAKEFLAGS        := --no-print-directory
export ASFLAGS          := $(AS_WARN)
export CFLAGS           := $(CC_WARN)
export LDFLAGS          :=

ifdef _DEBUG
  export ASFLAGS        += $(AS_DEBUG_SYM) $(AS_DEBUG_DEFINE)
  export CFLAGS         += $(CC_DEBUG_SYM) $(CC_DEBUG_DEFINE)
else
  export ASFLAGS        += $(AS_OPT)
  export CFLAGS         += $(CC_OPT)
endif

#ifdef _X86
export ASFLAGS          += -m32
export CFLAGS           += -m32
#endif

export CFLAGS           += -ffreestanding

# TODO: assess whether these are needed
export CFLAGS           += -fno-exceptions \
                           -fno-unwind-tables \
                           -fno-asynchronous-unwind-tables

# ------------------------------------------------------------------------------
# Directory tree tracking
ifeq ($(_OSROOT),$(CURDIR))
  export TREE           :=
  FAKE_DIR              := 1
endif
ifeq ($(_SRCROOT),$(CURDIR))
  export TREE           :=
  FAKE_DIR              := 1
endif

ifndef FAKE_DIR
  export TREE           := $(subst $(_SRCROOT)/,,$(CURDIR))
endif

# ------------------------------------------------------------------------------
# Shell Commands
export MKDIR            := mkdir -p
export RM    	        := rm -f

# ------------------------------------------------------------------------------
# Build directories, include paths, source files, etc.
export BINDIR           := $(_BINROOT)
export OBJDIR           := $(_OBJROOT)/$(TREE)
export INC		:= . \
                           $(_SRCROOT)/include
export SRC              = $(wildcard *.c)
export OBJ              = $(addprefix $(OBJDIR)/,$(SRC:.c=.o))
export DEP              = $(OBJ:%.o=%.d)
# export BIN	        :=

# Binutils/Compiler
export BINUTILS_PREFIX  := i686-elf-
export AS               := $(BINUTILS_PREFIX)gcc
export CC               := $(BINUTILS_PREFIX)gcc
export LD               := $(BINUTILS_PREFIX)ld
export OBJCOPY          := $(BINUTILS_PREFIX)objcopy

# ------------------------------------------------------------------------------
# Rules

.PHONY: all dirs clean debug-make

all: dirs $(OBJ) $(BIN)

dirs:
	@$(MKDIR) $(OBJDIR)
ifdef BIN
	@$(MKDIR) $(BINDIR)
endif

clean:
	@$(RM) -r $(BINDIR)
	@$(RM) -r $(OBJDIR)

debug-make:
	@echo '_OSROOT = $(_OSROOT)'
	@echo '_SRCROOT = $(_SRCROOT)'
	@echo '_BINROOT = $(_BINROOT)'
	@echo '_OBJROOT = $(_OBJROOT)'
	@echo '_SCRIPTS = $(_SCRIPTS)'
	@echo 'TREE = $(TREE)'
	@echo 'BINDIR = $(BINDIR)'
	@echo 'OBJDIR = $(OBJDIR)'
	@echo 'CFLAGS = $(CFLAGS)'
	@echo 'ASFLAGS = $(ASFLAGS)'
	@echo 'LDFLAGS = $(LDFLAGS)'
	@echo 'BIN = $(BIN)'
	@echo 'SRC = $(SRC)'
	@echo 'OBJ = $(OBJ)'
	@echo 'DEP = $(DEP)'
	@echo 'INC = $(INC)'

$(BINDIR)/%.bin: $(BINDIR)/%.elf
	@$(OBJCOPY) -O binary $< $@
	@echo 'OBJCOPY -O binary $< $@'

$(BINDIR)/%.sys: $(BINDIR)/%.elf
	@$(OBJCOPY) -O binary $< $@
	@echo 'OBJCOPY -O binary $< $@'

$(BINDIR)/%.elf: $(OBJ)
	@echo 'LD      $(LDFLAGS) $^'
	@$(LD) $(LDFLAGS) -o $@ $^

$(OBJDIR)/%.o: %.c
	@echo 'CC      $(CFLAGS) $(TREE)/$<'
	@$(CC) $(CFLAGS) $(addprefix -I,$(INC)) -MD -MF $(@:.o=.d) -c -o $@ $<

$(OBJDIR)/%.o: %.S
	@echo 'AS      $(ASFLAGS) $(TREE)/$<'
	@$(AS) $(ASFLAGS) $(addprefix -I,$(INC)) -MD -MF $(@:.o=.d) -c -o $@ $<

-include $(DEP)
