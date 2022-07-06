# Variables that can be overriden
# TARGET
# ARCH
# DEBUG
# ENTRY_POINT
# CODE_BASE
# DIRS
# C_FLAGS
# C_DEFINES
# C_WARNINGS
# AS_FLAGS
# AS_DEFINES
# AS_WARNINGS
# LD_FLAGS
# LD_WARNINGS

# ------------------------------------------------------------------------------
# Global config

export DEBUG            = 1
export ARCH             = x86

# ------------------------------------------------------------------------------
# Build environment sanity checking

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

# ------------------------------------------------------------------------------
# Directory tree tracking

ifeq ($(_OSROOT),$(CURDIR))
  TREEROOT = 1
  OSROOT = 1
endif
ifeq ($(_SRCROOT),$(CURDIR))
  TREEROOT = 1
  SRCROOT = 1
endif

ifndef TREEROOT
  export TREE           = $(subst $(_SRCROOT)/,,$(CURDIR))
endif

# ------------------------------------------------------------------------------
# Shell Commands

BINUTILS_PREFIX         := i686-elf-
export AS               := $(BINUTILS_PREFIX)gcc
export CC               := $(BINUTILS_PREFIX)gcc
export LD               := $(BINUTILS_PREFIX)ld
export OBJCOPY          := $(BINUTILS_PREFIX)objcopy
export MKDIR            := mkdir -p
export RM               := rm -f

# ------------------------------------------------------------------------------
# Directories

export BIN_PATH         := $(_BINROOT)/$(ARCH)
export OBJ_PATH         := $(_OBJROOT)/$(ARCH)

ifndef TREEROOT
  export OBJ_PATH       := $(OBJ_PATH)/$(TREE)
endif

# ------------------------------------------------------------------------------
# Includes

export INC              := . \
                          $(_SRCROOT)/include

# ------------------------------------------------------------------------------
# Binaries

TARGET_ELF               := $(addsuffix .elf, $(basename $(TARGET)))

# ------------------------------------------------------------------------------
# Sources, objects, and dependencies

SRC_S                   = $(wildcard *.S)
SRC_C                   = $(wildcard *.c)


ifdef SRC_S
  SRC                   += $(SRC_S)
  OBJ                   += $(addprefix $(OBJ_PATH)/,$(SRC_S:.S=.o))
endif
ifdef SRC_C
  SRC                   += $(SRC_C)
  OBJ                   += $(addprefix $(OBJ_PATH)/,$(SRC_C:.c=.o))
endif

DEP                     = $(OBJ:%.o=%.d)

# ------------------------------------------------------------------------------
# Defines, flags, and warnings

export C_FLAGS          =
export C_DEFINES        =
export C_WARNINGS       = all extra pedantic error
export AS_FLAGS         =
export AS_DEFINES       =
export AS_WARNINGS      = all extra pedantic error
export LD_FLAGS         =
export LD_WARNINGS      =
export OBJCOPY_FLAGS    = -Obinary

export MAKEFLAGS        = --no-print-directory

ifdef DEBUG
  C_DEFINES             += DEBUG
  AS_DEFINES            += DEBUG
endif

ifdef ENTRY_POINT
  LD_FLAGS              += -e $(ENTRY_POINT)
endif
ifdef CODE_BASE
  LD_FLAGS              += -Ttext $(CODE_BASE)
endif

CC_ARGS                 = $(addprefix -D,$(C_DEFINES)) $(addprefix -I,$(INC)) $(addprefix -W,$(C_WARNINGS)) $(C_FLAGS)
AS_ARGS                 = $(addprefix -D,$(AS_DEFINES)) $(addprefix -I,$(INC)) $(addprefix -W,$(AS_WARNINGS)) $(AS_FLAGS)
LD_ARGS                 = $(LD_FLAGS)

# ------------------------------------------------------------------------------
# Settings for this (root) Makefile

ifdef OSROOT
  DIRS                  = src
endif

# ------------------------------------------------------------------------------
# Rules

.PHONY: all clean nuke dirs debug-make $(DIRS)

all: dirs $(DIRS) $(TARGET)

clean:
	@$(RM) $(TARGET)
	@$(RM) $(TARGET_ELF)
	@$(RM) -r $(OBJ_PATH)

nuke:
	@$(RM) -r $(_OBJROOT)
	@$(RM) -r $(_BINROOT)

dirs:
	@$(MKDIR) $(BIN_PATH)
	@$(MKDIR) $(OBJ_PATH)

$(DIRS):
	@$(MAKE) -C $@

$(OBJ_PATH)/%.o: %.S
	@echo 'AS      $(AS_ARGS) $(TREE)/$<'
	@$(AS)         $(AS_ARGS) -MD -MF $(@:.o=.d) -c -o $@ $<

$(TARGET_ELF): $(OBJ)
	@$(MKDIR)      $(dir $(TARGET))
	@echo 'LD      $(LD_ARGS) $^'
	@$(LD)         $(LD_ARGS) -o $@ $^

$(TARGET): %: $(TARGET_ELF)
	@echo 'OBJCOPY -Obinary $< $@'
	@$(OBJCOPY)    -Obinary $< $@

debug-make:
	@echo '----------------------------------------'
	@echo ">>> Build Variables <<<"
	@echo '----------------------------------------'
	@echo '_OSROOT = $(_OSROOT)'
	@echo '_SRCROOT = $(_SRCROOT)'
	@echo '_BINROOT = $(_BINROOT)'
	@echo '_OBJROOT = $(_OBJROOT)'
	@echo '_SCRIPTS = $(_SCRIPTS)'
	@echo '_TOOLSRC = $(_TOOLSRC)'
	@echo '----------------------------------------'
	@echo 'ARCH = $(ARCH)'
	@echo 'DEBUG = $(DEBUG)'
	@echo '----------------------------------------'
	@echo 'TREE = $(TREE)'
	@echo 'DIRS = $(DIRS)'
	@echo 'BIN_PATH = $(BIN_PATH)'
	@echo 'OBJ_PATH = $(OBJ_PATH)'
	@echo 'TARGET = $(TARGET)'
	@echo 'TARGET_ELF = $(TARGET_ELF)'
	@echo '----------------------------------------'
	@echo 'C_FLAGS = $(C_FLAGS)'
	@echo 'C_DEFINES = $(C_DEFINES)'
	@echo 'C_WARNINGS = $(C_WARNINGS)'
	@echo 'AS_FLAGS = $(AS_FLAGS)'
	@echo 'AS_DEFINES = $(AS_DEFINES)'
	@echo 'AS_WARNINGS = $(AS_WARNINGS)'
	@echo 'LD_FLAGS = $(LD_FLAGS)'
	@echo 'LD_WARNINGS = $(LD_WARNINGS)'
	@echo 'OBJCOPY_FLAGS = $(OBJCOPY_FLAGS)'
	@echo 'MAKEFLAGS = $(MAKEFLAGS)'
	@echo '----------------------------------------'
	@echo 'CC = $(CC)'
	@echo 'AS = $(AS)'
	@echo 'LD = $(LD)'
	@echo 'OBJCOPY = $(OBJCOPY)'
	@echo 'MAKE = $(MAKE)'
	@echo 'MKDIR = $(MKDIR)'
	@echo 'RM = $(RM)'
	@echo '----------------------------------------'
	@echo 'CC_ARGS = $(CC_ARGS)'
	@echo 'AS_ARGS = $(AS_ARGS)'
	@echo 'LD_ARGS = $(LD_ARGS)'
	@echo '----------------------------------------'
	@echo 'INC = $(INC)'
	@echo 'SRC = $(SRC)'
	@echo 'OBJ = $(OBJ)'
	@echo 'DEP = $(DEP)'

# Include dependency rules
-include $(DEP)
