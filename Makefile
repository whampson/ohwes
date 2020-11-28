#==============================================================================#
# Copyright (C) 2020 Wes Hampson. All Rights Reserved.                         #
#                                                                              #
# This file is part of the Niobium Operating System.                           #
# Niobium is free software; you may redistribute it and/or modify it under     #
# the terms of the license agreement provided with this software.              #
#==============================================================================#

# i686-elf-tools can be found here: https://github.com/lordmilko/i686-elf-tools

export AS			:= i686-elf-as
export ASFLAGS		:=
export CC			:= i686-elf-gcc
export CFLAGS		:=
export LD			:= i686-elf-ld
export LDFLAGS		:=
export MAKEFLAGS	:= --no-print-directory

export MKDIR		:= mkdir -p
export RM			:= rm -f

export TREE 		:=
export TOPDIR 		:= $(CURDIR)
export BINDIR		:= $(TOPDIR)/bin
export OBJDIR		:= $(TOPDIR)/obj

.PHONY: all tools clean-tools boot

all: tools boot

clean-tools:
	@$(RM) -r tools/bin
	@$(RM) -r tools/obj

tools: dirs
	@$(MAKE) -C tools

boot: dirs
	@$(MAKE) -C boot


include $(TOPDIR)/Rules.mk
