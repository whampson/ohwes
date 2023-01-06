#===============================================================================
#    File: Makefile
# Created: July 4, 2022
#  Author: Wes Hampson
#
# The master Makefile containing the build system infrastructure.
# All Makefiles in this project must include this Makefile in order for the
# build system to function properly. This can be done by adding the following
# line in your Makefile:
#     include $(_MAKEROOT)
#
# _MAKEROOT is an environment variable that contains the path to this file.
#===============================================================================

# !!! TODO: header file change detection appears to be broken (MinGW)

ifndef _MAKEROOT
  $(error "Please source src/scripts/env.sh before invoking this Makefile.")
endif

# ------------------------------------------------------------------------------
# Global config

export DEBUG            = 1
export ARCH             = x86

# ------------------------------------------------------------------------------
# Makefile build OS detection
# https://stackoverflow.com/a/14777895

ifeq ($(OS),Windows_NT)
  BUILD_OS := Windows
else
  BUILD_OS := $(shell sh -c 'uname 2>/dev/null || echo Unknown')
endif

ifeq ($(BUILD_OS),Windows)
  WIN32 = 1
else ifeq ($(BUILD_OS),Darwin)
  OSX = 1
else ifeq ($(BUILD_OS),Linux)
  LINUX = 1
else
  $(error Unknown system type! Cannot build.)
endif


# ------------------------------------------------------------------------------
# Directory tree tracking

ifeq ($(_OSROOT),$(CURDIR))
  __TREEROOT = 1
  __OSROOT = 1
endif
ifeq ($(_SRCROOT),$(CURDIR))
  __TREEROOT = 1
  __SRCROOT = 1
endif

ifndef __TREEROOT
  export TREE           = $(subst $(_SRCROOT)/,,$(CURDIR))
endif

# ------------------------------------------------------------------------------
# Shell Commands

BINUTILS_PREFIX         := i686-elf-
export AS               := $(BINUTILS_PREFIX)gcc
export CC               := $(BINUTILS_PREFIX)gcc
export CXX              := $(BINUTILS_PREFIX)g++
export LD               := $(BINUTILS_PREFIX)ld
export OBJCOPY          := $(BINUTILS_PREFIX)objcopy
export MKDIR            := mkdir -p
export RM               := rm -f
export COPY             := cp -f

# ------------------------------------------------------------------------------
# Directories

ifndef BIN_PATH
  export BIN_PATH       := $(_BINROOT)/$(ARCH)
endif
ifndef OBJ_PATH
  export OBJ_PATH       := $(_OBJROOT)/$(ARCH)
  ifndef __TREEROOT
    export OBJ_PATH     := $(OBJ_PATH)/$(TREE)
  endif
endif

# ------------------------------------------------------------------------------
# Includes

ifndef INC
  export INC            := . \
                          $(_SRCROOT)/include
endif

# ------------------------------------------------------------------------------
# Binaries

TARGET_ELF              = $(addsuffix .elf, $(basename $(TARGET)))

# ------------------------------------------------------------------------------
# Sources, objects, and dependencies

__SRC_S                 = $(wildcard *.S)
__SRC_C                 = $(wildcard *.c)
__SRC_CPP               = $(wildcard *.cpp)

ifdef __SRC_S
  SRC                   += $(__SRC_S)
  OBJ                   += $(addprefix $(OBJ_PATH)/,$(__SRC_S:.S=.o))
endif
ifdef __SRC_C
  SRC                   += $(__SRC_C)
  OBJ                   += $(addprefix $(OBJ_PATH)/,$(__SRC_C:.c=.o))
endif
ifdef __SRC_CPP
  SRC                   += $(__SRC_CPP)
  OBJ                   += $(addprefix $(OBJ_PATH)/,$(__SRC_CPP:.cpp=.o))
endif

DEP                     = $(OBJ:.o=.d)

# ------------------------------------------------------------------------------
# Defines, flags, and warnings
# -W, -D, -I automatically appended to warnings, defines, includes

export C_FLAGS          = -std=c11
export C_DEFINES        =
export C_WARNINGS       = all extra error
export CXX_FLAGS        = -std=c++11
export CXX_DEFINES      =
export CXX_WARNINGS     = all extra error
export AS_FLAGS         =
export AS_DEFINES       =
export AS_WARNINGS      = all extra error
export LD_FLAGS         =
export LD_WARNINGS      =
export OBJCOPY_FLAGS    = -Obinary

MAKEFLAGS               = --no-print-directory

ifndef WIN32
  C_WARNINGS += pedantic
  CXX_WARNINGS += pedantic
  AS_WARNINGS += pedantic
endif

ifdef DEBUG
  C_DEFINES             += DEBUG
  CXX_DEFINES           += DEBUG
  AS_DEFINES            += DEBUG

  C_FLAGS               += -g
  CXX_FLAGS             += -g
endif

ifdef ENTRY_POINT
  LD_FLAGS              += -e $(ENTRY_POINT)
endif
ifdef CODE_BASE
  LD_FLAGS              += -Ttext $(CODE_BASE)
endif

CC_ARGS                 = $(addprefix -D,$(C_DEFINES)) $(addprefix -I,$(INC)) $(addprefix -W,$(C_WARNINGS)) $(C_FLAGS)
CXX_ARGS                = $(addprefix -D,$(CXX_DEFINES)) $(addprefix -I,$(INC)) $(addprefix -W,$(CXX_WARNINGS)) $(CXX_FLAGS)
AS_ARGS                 = $(addprefix -D,$(AS_DEFINES)) $(addprefix -I,$(INC)) $(addprefix -W,$(AS_WARNINGS)) $(AS_FLAGS)
LD_ARGS                 = $(LD_FLAGS)
OBJCOPY_ARGS            = $(OBJCOPY_FLAGS)

# ------------------------------------------------------------------------------
# Settings for this (root) Makefile

ifdef __OSROOT
  DIRS                  = src
endif

# ------------------------------------------------------------------------------
# Rules

.PHONY: all remake clean nuke dirs debug-make $(DIRS) img
.DEFAULT_GOAL: all

ifdef NO_ELF
  .INTERMEDIATE: $(TARGET_ELF)
endif

all: dirs $(DIRS) $(TARGET)

remake: clean all

clean:
	@$(RM) $(TARGET)
	@$(RM) $(TARGET_ELF)
	@$(RM) -r $(OBJ_PATH)

nuke:
	@$(RM) -r $(_OBJROOT)
	@$(RM) -r $(_BINROOT)

img:
	@fatfs -p create --force $(_BINROOT)/ohwes.img

dirs:
	@$(MKDIR) $(BIN_PATH)
	@$(MKDIR) $(OBJ_PATH)

$(DIRS):
	@$(MAKE) -C $@

$(TARGET): %: $(TARGET_ELF) $(OBJ)
ifndef NO_OBJCOPY
	@echo 'OBJCOPY $(OBJCOPY_ARGS) $< $@'
	@$(OBJCOPY)    $(OBJCOPY_ARGS) $< $@
else
	@echo 'COPY    $< $@'
	@$(COPY)       $< $@
endif

$(TARGET_ELF): $(OBJ)
	@$(MKDIR)      $(dir $(TARGET))
	@echo 'LD      $(LD_ARGS) -o $@ $^'
	@$(LD)         $(LD_ARGS) -o $@ $^

$(OBJ_PATH)/%.o: %.c
	@echo 'CC      $(CC_ARGS) -c -MD -MF $(@:.o=.d) -o $@ $(TREE)/$<'
	@$(CC)         $(CC_ARGS) -c -MD -MF $(@:.o=.d) -o $@ $<

$(OBJ_PATH)/%.o: %.cpp
	@echo 'CXX     $(CXX_ARGS) -c -MD -MF $(@:.o=.d) -o $@ $(TREE)/$<'
	@$(CXX)        $(CXX_ARGS) -c -MD -MF $(@:.o=.d) -o $@ $<

$(OBJ_PATH)/%.o: %.S
	@echo 'AS      $(AS_ARGS) -c -MD -MF $(@:.o=.d) -o $@ $(TREE)/$<'
	@$(AS)         $(AS_ARGS) -c -MD -MF $(@:.o=.d) -o $@ $<

debug-make:
	@echo '----------------------------------------'
	@echo ">>> Build Variables <<<"
	@echo '----------------------------------------'
	@echo 'DIRS = $(DIRS)'
	@echo 'TARGET = $(TARGET)'
	@echo 'TREE = $(TREE)'
	@echo '----------------------------------------'
	@echo '_OSROOT = $(_OSROOT)'
	@echo '_SRCROOT = $(_SRCROOT)'
	@echo '_BINROOT = $(_BINROOT)'
	@echo '_OBJROOT = $(_OBJROOT)'
	@echo '_SCRIPTS = $(_SCRIPTS)'
	@echo '_TOOLSRC = $(_TOOLSRC)'
	@echo '_TOOLBIN = $(_TOOLBIN)'
	@echo '----------------------------------------'
	@echo '_MAKEROOT = $(_MAKEROOT)'
	@echo '_TOOLMAKEROOT = $(_TOOLMAKEROOT)'
	@echo '----------------------------------------'
	@echo 'BUILD_OS = $(BUILD_OS)'
	@echo 'WIN32 = $(WIN32)'
	@echo 'OSX = $(OSX)'
	@echo 'LINUX = $(LINUX)'
	@echo 'ARCH = $(ARCH)'
	@echo 'DEBUG = $(DEBUG)'
	@echo '----------------------------------------'
	@echo 'BIN_PATH = $(BIN_PATH)'
	@echo 'OBJ_PATH = $(OBJ_PATH)'
	@echo 'TARGET_ELF = $(TARGET_ELF)'
	@echo '----------------------------------------'
	@echo 'AS_FLAGS = $(AS_FLAGS)'
	@echo 'AS_DEFINES = $(AS_DEFINES)'
	@echo 'AS_WARNINGS = $(AS_WARNINGS)'
	@echo 'C_FLAGS = $(C_FLAGS)'
	@echo 'C_DEFINES = $(C_DEFINES)'
	@echo 'C_WARNINGS = $(C_WARNINGS)'
	@echo 'CXX_FLAGS = $(CXX_FLAGS)'
	@echo 'CXX_DEFINES = $(CXX_DEFINES)'
	@echo 'CXX_WARNINGS = $(CXX_WARNINGS)'
	@echo 'LD_FLAGS = $(LD_FLAGS)'
	@echo 'LD_WARNINGS = $(LD_WARNINGS)'
	@echo 'OBJCOPY_FLAGS = $(OBJCOPY_FLAGS)'
	@echo 'MAKEFLAGS = $(MAKEFLAGS)'
	@echo '----------------------------------------'
	@echo 'AS = $(AS)'
	@echo 'CC = $(CC)'
	@echo 'CXX = $(CXX)'
	@echo 'LD = $(LD)'
	@echo 'OBJCOPY = $(OBJCOPY)'
	@echo 'MAKE = $(MAKE)'
	@echo 'MKDIR = $(MKDIR)'
	@echo 'RM = $(RM)'
	@echo 'COPY = $(COPY)'
	@echo '----------------------------------------'
	@echo 'AS_ARGS = $(AS_ARGS)'
	@echo 'CC_ARGS = $(CC_ARGS)'
	@echo 'CXX_ARGS = $(CXX_ARGS)'
	@echo 'LD_ARGS = $(LD_ARGS)'
	@echo '----------------------------------------'
	@echo 'INC = $(INC)'
	@echo 'SRC = $(SRC)'
	@echo 'OBJ = $(OBJ)'
	@echo 'DEP = $(DEP)'

# Include dependency rules
-include $(DEP)
