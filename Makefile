#==============================================================================#
# Copyright (C) 2020 Wes Hampson. All Rights Reserved.                         #
#                                                                              #
# This file is part of the Niobium Operating System.                           #
# Niobium is free software; you may redistribute it and/or modify it under     #
# the terms of the license agreement provided with this software.              #
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
#------------------------------------------------------------------------------#
# Master Makefile for the Niobium Operating System and associated tools.       #
#                                                                              #
# Niobium is currently built using GCC 7.1.0 and Binutils 2.28 configured for  #
# i686-elf binaries. To build, you must first add these tools to your system's #
# PATH. You can find a precompiled set of the i686-elf tools here:             #
#     https://github.com/lordmilko/i686-elf-tools                              #
# The programs in tools/ are built using your system's native GCC and Binutils #
# as they are not meant to be run on Niobium.                                  #
#==============================================================================#

export AS			:= i686-elf-as
export ASFLAGS		:=
export CC			:= i686-elf-gcc
export CFLAGS		:= -g
export LD			:= i686-elf-ld
export LDFLAGS		:=
export MAKEFLAGS	:= --no-print-directory

export MKDIR		:= mkdir -p
export RM			:= rm -f

export TREE			:=
export TOPDIR		:= $(CURDIR)
export BINDIR		:= $(TOPDIR)/bin
export OBJDIR		:= $(TOPDIR)/obj
export IMGDIR		:= $(BINDIR)/img
export IMGFILE		:= $(IMGDIR)/niobium.img

.PHONY: all wipe tools clean-tools img fatfs boot

all: img

img: boot tools
	@$(MKDIR) -p $(IMGDIR)
	@fatfs $(IMGFILE) create
	@fatfs $(IMGFILE) add $(BINDIR)/init.sys
	@fatfs $(IMGFILE) add $(BINDIR)/kernel.sys
	@dd if=$(BINDIR)/bootsect of=$(IMGFILE) conv=notrunc status=none
	@echo 'OUT $(subst $(TOPDIR)/,,$(IMGFILE))'

boot: dirs
	@$(MAKE) -C boot

tools: dirs
	@$(MAKE) -C tools

fatfs: dirs
	@$(MAKE) -C tools fatfs

wipe: clean clean-tools

clean-tools:
	@$(RM) -r tools/bin
	@$(RM) -r tools/obj

include $(TOPDIR)/include/Rules.mk
