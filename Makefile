#===============================================================================
#    File: Makefile
# Created: July 4, 2022
#  Author: Wes Hampson
#
# The master Makefile containing the build system infrastructure. All Makefiles
# in this project must 'include' this Makefile in order for the build system to
# function properly. This can be done by adding the following line in your
# Makefile:
#     include $(MAKEROOT)
# MAKEROOT is an environment variable that contains the path to this file.
# The location of this line in your Makefile will affect the build behavior.
# Please define all prerequisite variables, such as SOURCES, DIRS, and TARGET,
# before including MAKEROOT.
#===============================================================================

ifndef MAKEROOT
  $(error "Build environment not set! Please source src/scripts/env.sh before invoking this Makefile.")
endif

# ------------------------------------------------------------------------------
# Global config

export ARCH  = x86
export DEBUG = 1

# ------------------------------------------------------------------------------
# Makefile build OS detection
# https://stackoverflow.com/a/14777895

ifeq '$(findstring :,$(PATH))' ';'
  BUILD_OS := Windows
else
  BUILD_OS := $(shell uname 2>/dev/null || echo Unknown)
  BUILD_OS := $(patsubst CYGWIN%,Cygwin,$(BUILD_OS))
  BUILD_OS := $(patsubst MSYS%,MSYS,$(BUILD_OS))
  BUILD_OS := $(patsubst MINGW%,MSYS,$(BUILD_OS))
endif

ifeq ($(BUILD_OS),MSYS)
  MSYS     = 1
  DEFINES += MSYS
else ifeq ($(BUILD_OS),Darwin)
  OSX      = 1
  DEFINES += OSX
else
  $(warning Unsupported system type!)
endif

# ------------------------------------------------------------------------------
# Directory tree tracking

ifeq ($(OSROOT),$(CURDIR))
  TREEROOT = 1
  IN_OSROOT = 1
endif
ifeq ($(SRCROOT),$(CURDIR))
  TREEROOT = 1
endif

ifndef TREEROOT
  export TREE = $(subst $(SRCROOT)/,,$(CURDIR))
endif

# ------------------------------------------------------------------------------
# Directories

ifndef BINDIR
  export BINDIR     := $(BINROOT)/$(ARCH)
endif
ifndef OBJDIR
  export OBJDIR     := $(OBJROOT)
  ifndef TREEROOT
    export OBJDIR   := $(OBJDIR)/$(TREE)
  endif
endif

# ------------------------------------------------------------------------------
# Shell Commands

BINUTILS_PREFIX     := i686-elf-
export AS           := $(BINUTILS_PREFIX)gcc
export CC           := $(BINUTILS_PREFIX)gcc
export CXX          := $(BINUTILS_PREFIX)g++
export LD           := $(BINUTILS_PREFIX)ld
export OBJCOPY      := $(BINUTILS_PREFIX)objcopy
export MKDIR        := mkdir -p
export RM           := rm -f
export COPY         := cp -f

# ------------------------------------------------------------------------------
# Includes

ifndef INCLUDES
  export INCLUDES   := . $(SRCROOT)/include
endif

# ------------------------------------------------------------------------------
# Sources, objects, and dependencies

SOURCES_S           = $(filter %.S,$(SOURCES))
SOURCES_C           = $(filter %.c,$(SOURCES))
SOURCES_CPP         = $(filter %.cpp,$(SOURCES))

ifdef SOURCES_S
  OBJECTS           += $(addprefix $(OBJDIR)/,$(SOURCES_S:.S=.o))
endif
ifdef SOURCES_C
  OBJECTS           += $(addprefix $(OBJDIR)/,$(SOURCES_C:.c=.o))
endif
ifdef SOURCES_CPP
  OBJECTS           += $(addprefix $(OBJDIR)/,$(SOURCES_CPP:.cpp=.o))
endif

DEPENDS             = $(OBJECTS:.o=.d)

# ------------------------------------------------------------------------------
# Binaries

ifdef TARGET
  TARGET              := $(BINDIR)/$(TARGET)
  TARGET_ELF          = $(addsuffix .elf, $(basename $(TARGET)))
endif

# ------------------------------------------------------------------------------
# Defines, flags, and warnings
# -W, -D, -I automatically appended to warnings, defines, includes respectively.

MAKEFLAGS           += --no-print-directory

export CFLAGS       += -std=c11
export CXXFLAGS     += -std=c++11
export LDFLAGS      +=

export CC_WARNINGS  += all extra error
export CXX_WARNINGS += all extra error
export AS_WARNINGS  += all extra error

ifndef MSYS
  CC_WARNINGS       += pedantic
  CXX_WARNINGS      += pedantic
  AS_WARNINGS       += pedantic
endif

ifdef DEBUG
  DEFINES           += DEBUG
  CFLAGS            += -g
  CXXFLAGS          += -g
  ASFLAGS           += -g
  LDFLAGS           += -g
endif

ifdef ENTRYPOINT
  LDFLAGS           += -e $(ENTRYPOINT)
endif
ifdef BASE
  LDFLAGS           += -Ttext $(BASE)
endif

CC_ARGS             = $(addprefix -D,$(DEFINES)) $(addprefix -I,$(INCLUDES)) $(addprefix -W,$(CC_WARNINGS)) $(CFLAGS)
CXX_ARGS            = $(addprefix -D,$(DEFINES)) $(addprefix -I,$(INCLUDES)) $(addprefix -W,$(CXX_WARNINGS)) $(CXXFLAGS)
AS_ARGS             = $(addprefix -D,$(DEFINES)) $(addprefix -I,$(INCLUDES)) $(addprefix -W,$(AS_WARNINGS)) $(ASFLAGS)
LD_ARGS             = $(LDFLAGS)
OBJCOPY_ARGS        = -Obinary

# ------------------------------------------------------------------------------
# Settings for this (root) Makefile

ifdef IN_OSROOT
  DIRS         = src
else
  ifndef SOURCES
    ifndef DIRS
      $(error Need to define either SOURCES or DIRS!)
    endif
  endif
  ifndef DIRS
    ifndef SOURCES
      $(error Need to define either SOURCES or DIRS!)
    endif
  endif
endif

# ------------------------------------------------------------------------------
# Rules

.PHONY: all clean nuke remake dirs $(DIRS) img tools debug-make
.DEFAULT_GOAL: all

ifdef NO_ELF
  .INTERMEDIATE: $(TARGET_ELF)
endif

# Do it all!!
all:: dirs $(DIRS) $(TARGET)

# Delete objects and target binaries for current build tree.
clean::
	@$(RM) $(TARGET)
	@$(RM) $(TARGET_ELF)
	@$(RM) -r $(OBJDIR)

# Delete ALL objects and target binaries.
nuke::
	@$(RM) -r $(OBJROOT)
	@$(RM) -r $(BINROOT)


# Clean and rebuild the current tree.
remake:: clean all

# Make binary and object directories.
dirs::
	@$(MKDIR) $(BINDIR)
	@$(MKDIR) $(OBJDIR)

# Invoke the Makefiles in each directory on the DIRS list.
$(DIRS)::
	@$(MAKE) -C $@

# Make the OS disk image.
img:: tools
	fatfs -pq create --force $(BINROOT)/ohwes.img
	dd if=$(BINROOT)/$(ARCH)/boot/boot.bin of=$(BINROOT)/ohwes.img bs=512 count=1 conv=notrunc status=none
	fatfs -p info $(BINROOT)/ohwes.img

# Build tools.
tools::
	@$(MAKE) -C $(TOOLSSRC)

$(TARGET): %: $(TARGET_ELF) $(OBJECTS)
ifndef NO_OBJCOPY
	@echo 'OBJCOPY $(OBJCOPY_ARGS) $< $@'
	@$(OBJCOPY)    $(OBJCOPY_ARGS) $< $@
else
	@echo 'COPY    $< $@'
	@$(COPY)       $< $@
endif

$(TARGET_ELF): $(OBJECTS)
	@$(MKDIR)      $(dir $(TARGET))
	@echo 'LD      $(LD_ARGS) -o $@ $^'
	@$(LD)         $(LD_ARGS) -o $@ $^

$(OBJDIR)/%.o: %.c
	@echo 'CC      $(CC_ARGS) -c -MD -MF $(@:.o=.d) -o $@ $(TREE)/$<'
	@$(CC)         $(CC_ARGS) -c -MD -MF $(@:.o=.d) -o $@ $<

$(OBJDIR)/%.o: %.cpp
	@echo 'CXX     $(CXX_ARGS) -c -MD -MF $(@:.o=.d) -o $@ $(TREE)/$<'
	@$(CXX)        $(CXX_ARGS) -c -MD -MF $(@:.o=.d) -o $@ $<

$(OBJDIR)/%.o: %.S
	@echo 'AS      $(AS_ARGS) -c -MD -MF $(@:.o=.d) -o $@ $(TREE)/$<'
	@$(AS)         $(AS_ARGS) -c -MD -MF $(@:.o=.d) -o $@ $<

debug-make:
	@echo '----------------------------------------'
	@echo ">>> Build Variables <<<"
	@echo '----------------------------------------'
	@echo '      CURDIR = $(CURDIR)'
	@echo '      OSROOT = $(OSROOT)'
	@echo '    MAKEROOT = $(MAKEROOT)'
	@echo '     SRCROOT = $(SRCROOT)'
	@echo '     BINROOT = $(BINROOT)'
	@echo '     OBJROOT = $(OBJROOT)'
	@echo '     SCRIPTS = $(SCRIPTS)'
	@echo '    TOOLSSRC = $(TOOLSSRC)'
	@echo '    TOOLSBIN = $(TOOLSBIN)'
	@echo '----------------------------------------'
	@echo '    BUILD_OS = $(BUILD_OS)'
	@echo '        MSYS = $(MSYS)'
	@echo '         OSX = $(OSX)'
	@echo '----------------------------------------'
	@echo '        ARCH = $(ARCH)'
	@echo '       DEBUG = $(DEBUG)'
	@echo '----------------------------------------'
	@echo '        TREE = $(TREE)'
	@echo '      TARGET = $(TARGET)'
	@echo '  TARGET_ELF = $(TARGET_ELF)'
	@echo '      BINDIR = $(BINDIR)'
	@echo '      OBJDIR = $(OBJDIR)'
	@echo '        DIRS = $(DIRS)'
	@echo '     DEFINES = $(DEFINES)'
	@echo '    INCLUDES = $(INCLUDES)'
	@echo '     SOURCES = $(SOURCES)'
	@echo '     OBJECTS = $(OBJECTS)'
	@echo '----------------------------------------'
	@echo '          CC = $(CC)'
	@echo '     CC_ARGS = $(CC_ARGS)'
	@echo '----------------------------------------'
	@echo '         CXX = $(CXX)'
	@echo '    CXX_ARGS = $(CXX_ARGS)'
	@echo '----------------------------------------'
	@echo '          AS = $(AS)'
	@echo '     AS_ARGS = $(AS_ARGS)'
	@echo '----------------------------------------'
	@echo '          LD = $(LD)'
	@echo '     LD_ARGS = $(LD_ARGS)'
	@echo '----------------------------------------'
	@echo '        COPY = $(COPY)'
	@echo '        MAKE = $(MAKE)'
	@echo '   MAKEFLAGS = $(MAKEFLAGS)'
	@echo '       MKDIR = $(MKDIR)'
	@echo '     OBJCOPY = $(OBJCOPY)'
	@echo '          RM = $(RM)'
	@echo '----------------------------------------'

# Include dependency rules
-include $(DEPENDS)
