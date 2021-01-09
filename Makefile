#==============================================================================#
# Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                    #
#                                                                              #
# This file is part of the OHWES Operating System.                             #
# OHWES is free software; you may redistribute it and/or modify it under the   #
# terms of the license agreement provided with this software.                  #
#                                                                              #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL      #
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER   #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      #
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER          #
# DEALINGS IN THE SOFTWARE.                                                    #
#==============================================================================#
#    File: Makefile                                                            #
# Created: November 24, 2020                                                   #
#  Author: Wes Hampson                                                         #
#                                                                              #
# Master Makefile for OHWES and associated tools.                              #
#                                                                              #
# OHWES is currently built using GCC 7.1.0 and Binutils 2.28 configured for    #
# i686-elf binaries. To build, you must first add these tools to your system's #
# PATH. You can find a precompiled set of the i686-elf tools here:             #
#     https://github.com/lordmilko/i686-elf-tools                              #
# The programs in tools/ are built using your system's native GCC and Binutils #
# as they are not meant to be run on OHWES.                                    #
#==============================================================================#

export TREE		:=
export TOPDIR		:= $(CURDIR)
export OBJDIR		:= obj
export OBJBASE		:= $(TOPDIR)/$(OBJDIR)
export BINDIR		:= $(TOPDIR)/bin
export INCLUDE		:= $(TOPDIR)/include
export IMGFILE		:= $(BINDIR)/ohwes.img

BINUTILS_PREFIX		:= i686-elf-
GCC_WARNINGS		:= -Wall -Wextra -Werror -Wpedantic
GCC_FLAGS		:= $(GCC_WARNINGS) -g -m32

export AS		:= $(BINUTILS_PREFIX)gcc
export ASFLAGS		:= $(GCC_FLAGS)
export CC		:= $(BINUTILS_PREFIX)gcc
export CFLAGS		:= $(GCC_FLAGS) -ffreestanding -fno-exceptions \
			-fno-unwind-tables -fno-asynchronous-unwind-tables
export LD		:= $(BINUTILS_PREFIX)ld
export LDFLAGS		:=
export MAKEFLAGS	:= --no-print-directory
export MKDIR		:= mkdir -p
export OBJCOPY		:= $(BINUTILS_PREFIX)objcopy
export RM		:= rm -f

.PHONY: all img boot drivers kernel lib tools wipe clean-tools

all: tools img

img: boot kernel
	@echo '> Creating floppy image...'
	@fatfs -i $(IMGFILE) create
	@fatfs -i $(IMGFILE) add $(BINDIR)/init.sys
	@fatfs -i $(IMGFILE) add $(BINDIR)/ohwes.sys
	@echo '> Writing boot sector...'
	@dd if=$(BINDIR)/boot.bin of=$(IMGFILE) conv=notrunc status=none
	@echo 'OUT $(subst $(TOPDIR)/,,$(abspath $(IMGFILE)))'

boot: dirs
	@$(MAKE) -C boot

kernel: dirs lib drivers tests
	@$(MAKE) -C kernel

drivers: dirs
	@$(MAKE) -C drivers

lib: dirs
	@$(MAKE) -C lib

tests: dirs
	@$(MAKE) -C tests

tools: dirs
	@$(MAKE) -C tools

wipe: clean clean-tools

clean-tools:
	@$(RM) -r tools/bin
	@$(RM) -r tools/obj

include $(TOPDIR)/Rules.mk
