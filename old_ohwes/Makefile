# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# NIOBIUM NIOBIUM NIOBIUM NIOBIUM NIOBIUM NIOBIUM NIOBIUM NIOBIUM 
# NbOS NbOS NbOS NbOS NbOS NbOS NbOS NbOS NbOS NbOS NbOS NbOS NbOS
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

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

# ------------------------------------------------------------------------------
# Flags

export DEFINES          :=
export WARNINGS         := all extra error pedantic

export ASFLAGS          := -m32
export CFLAGS           := -m32
export LDFLAGS          :=
export MAKEFLAGS        := --no-print-directory

ifdef DEBUG
  export ASFLAGS        += -ggdb
  export CFLAGS         += -ggdb
  export DEFINES        += DEBUG
else
  # TODO: optimization flags?
  export ASFLAGS        +=
  export CFLAGS         +=
endif

export CFLAGS           += -ffreestanding \
                           -nostdinc \
                           -nostdlib

# TODO: assess whether these are needed
export CFLAGS           += -fno-exceptions \
                           -fno-unwind-tables \
                           -fno-asynchronous-unwind-tables

# ------------------------------------------------------------------------------
# Directory tree tracking

ifeq ($(_OSROOT),$(CURDIR))
  export TREE           =
  TREE_ROOT = 1
endif
ifeq ($(_SRCROOT),$(CURDIR))
  export TREE           =
  TREE_ROOT = 1
endif

ifndef TREE_ROOT
  export TREE           = $(subst $(_SRCROOT)/,,$(CURDIR))
endif

# ------------------------------------------------------------------------------
# Build directories, include paths, source files, etc.

export BINDIR           = $(_BINROOT)
export OBJDIR           = $(_OBJROOT)/$(TREE)

export INC		= . \
                           $(_SRCROOT)/include

SRC_S   = $(wildcard *.S)
SRC_C   = $(wildcard *.c)
SRC_CPP = $(wildcard *.cpp)

ifdef SRC_S
  export SRC            += $(SRC_S)
  export OBJ            += $(addprefix $(OBJDIR)/,$(SRC_S:.S=.o))
endif
ifdef SRC_C
  export SRC            += $(SRC_C)
  export OBJ            += $(addprefix $(OBJDIR)/,$(SRC_C:.c=.o))
endif
ifdef SRC_CPP
  export SRC            += $(SRC_CPP)
  export OBJ            += $(addprefix $(OBJDIR)/,$(SRC_CPP:.cpp=.o))
endif

export DEP              = $(OBJ:%.o=%.d)

# Binutils/Compiler
export BINUTILS_PREFIX  := i686-elf-
export AS               := $(BINUTILS_PREFIX)gcc
export CC               := $(BINUTILS_PREFIX)gcc
export LD               := $(BINUTILS_PREFIX)ld
export OBJCOPY          := $(BINUTILS_PREFIX)objcopy

# ------------------------------------------------------------------------------
# Shell Commands

export MKDIR            := mkdir -p
export RM    	        := rm -f

# ------------------------------------------------------------------------------
# Rules

.PHONY: all dirs src clean clean-all debug-make

# Default rule. Keep this as-is to ensure child Makefiles default to a working build.
all: dirs $(OBJ) $(BIN)

# Create directories.
dirs:
	@$(MKDIR) $(BINDIR)
	@$(MKDIR) $(OBJDIR)

# Build source.
src:
	$(MAKE) -C $(_SRCROOT)

# Clean objects from current working tree.
clean:
	@$(RM) -r $(OBJDIR)

# Clean objects and binaries from working tree.
clean-all:
	@$(RM) -r $(_OBJROOT)
	@$(RM) -r $(_BINROOT)

# Output some variables for debugging.
debug-make:
	@echo '_OSROOT = $(_OSROOT)'
	@echo '_SRCROOT = $(_SRCROOT)'
	@echo '_BINROOT = $(_BINROOT)'
	@echo '_OBJROOT = $(_OBJROOT)'
	@echo '_SCRIPTS = $(_SCRIPTS)'
	@echo '_TOOLSRC = $(_TOOLSRC)'
	@echo 'DEBUG = $(DEBUG)'
	@echo '----------------------------------------'
	@echo 'DEFINES = $(DEFINES)'
	@echo 'WARNINGS = $(WARNINGS)'
	@echo 'ASFLAGS = $(ASFLAGS)'
	@echo 'CFLAGS = $(CFLAGS)'
	@echo 'CXXFLAGS = $(CXXFLAGS)'
	@echo 'LDFLAGS = $(LDFLAGS)'
	@echo 'MAKEFLAGS = $(MAKEFLAGS)'
	@echo '----------------------------------------'
	@echo 'AS = $(AS)'
	@echo 'CC = $(CC)'
	@echo 'LD = $(LD)'
	@echo 'MAKE = $(MAKE)'
	@echo 'MKDIR = $(MKDIR)'
	@echo 'OBJCOPY = $(OBJCOPY)'
	@echo 'RM = $(RM)'
	@echo 'TREE = $(TREE)'
	@echo '----------------------------------------'
	@echo 'CURDIR = $(CURDIR)'
	@echo 'BINDIR = $(BINDIR)'
	@echo 'OBJDIR = $(OBJDIR)'
	@echo 'BIN = $(BIN)'
	@echo 'INC = $(INC)'
	@echo 'SRC = $(SRC)'
	@echo 'OBJ = $(OBJ)'
	@echo 'DEP = $(DEP)'

ARGS_AS  = $(addprefix -D,$(DEFINES)) $(addprefix -I,$(INC)) $(addprefix -W,$(WARNINGS)) $(ASFLAGS)
ARGS_CC  = $(addprefix -D,$(DEFINES)) $(addprefix -I,$(INC)) $(addprefix -W,$(WARNINGS)) $(CFLAGS)
ARGS_CXX = $(addprefix -D,$(DEFINES)) $(addprefix -I,$(INC)) $(addprefix -W,$(WARNINGS)) $(CXXFLAGS)

# Create object files from x86 ASM source.
$(OBJDIR)/%.o: %.S
	@echo 'AS      $(ARGS_AS) $(TREE)/$<'
	@$(AS)         $(ARGS_AS) -MD -MF $(@:.o=.d) -c -o $@ $<

# Create object files from C source.
$(OBJDIR)/%.o: %.c
	@echo 'CC      $(ARGS_CC) $(TREE)/$<'
	@$(CC)         $(ARGS_CC) -MD -MF $(@:.o=.d) -c -o $@ $<

# Create object files from C++ source.
$(OBJDIR)/%.o: %.cpp
	@echo 'CXX     $(ARGS_CXX) $(TREE)/$<'
	@$(CXX)        $(ARGS_CXX) -MD -MF $(@:.o=.d) -c -o $@ $<

# Create ELF executable files from object files.
$(BINDIR)/%.elf: $(OBJ)
	@echo 'LD      $(LDFLAGS) $^'
	@$(LD)         $(LDFLAGS) -o $@ $^

# Create raw binary files from ELF executables.
$(BINDIR)/%.bin: $(BINDIR)/%.elf
	@echo 'OBJCOPY -O binary $< $@'
	@$(OBJCOPY)    -O binary $< $@

# Create raw binary files from ELF executables (system driver files)
$(BINDIR)/%.sys: $(BINDIR)/%.elf
	@echo 'OBJCOPY -O binary $< $@'
	@$(OBJCOPY)    -O binary $< $@

# Include dependency rules
-include $(DEP)
