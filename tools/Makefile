#==============================================================================#
# Copyright (C) 2020 Wes Hampson. All Rights Reserved.                         #
#                                                                              #
# This file is part of the Niobium Operating System.                           #
# Niobium is free software; you may redistribute it and/or modify it under     #
# the terms of the license agreement provided with this software.              #
#==============================================================================#

ifeq ($(MAKELEVEL), 0)
$(error To build, please run 'make tools' from the top-level directory)
endif

include $(TOPDIR)/Tree.mk

export OBJDIR		:= $(CURDIR)/obj
export BINDIR		:= $(CURDIR)/bin

export CC			:= gcc
export CFLAGS		:=
export LD			:= gcc
export LDFLAGS		:=

.PHONY: all fatfs

all: fatfs

fatfs: dirs
	@$(MAKE) -C fatfs	

include $(TOPDIR)/Rules.mk