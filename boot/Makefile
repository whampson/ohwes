#==============================================================================#
# Copyright (C) 2020 Wes Hampson. All Rights Reserved.                         #
#                                                                              #
# This file is part of the Niobium Operating System.                           #
# Niobium is free software; you may redistribute it and/or modify it under     #
# the terms of the license agreement provided with this software.              #
#==============================================================================#

ifeq ($(MAKELEVEL), 0)
$(error To build, please run 'make boot' from the top-level directory)
endif

include $(TOPDIR)/Tree.mk

STAGE1_ENTRY		:= entry
STAGE1_OFFSET		:= 0x7C00
STAGE1_SOURCES		:= fat.S
STAGE1_OBJECTS		:= $(addprefix $(OBJDIR)/,$(STAGE1_SOURCES:.S=.o))
STAGE1_TARGETS		:= $(BINDIR)/bootsect

STAGE2_ENTRY		:= init
STAGE2_OFFSET		:= 0x8000
STAGE2_SOURCES		:= init.S
STAGE2_OBJECTS		:= $(addprefix $(OBJDIR)/,$(STAGE2_SOURCES:.S=.o))
STAGE2_TARGETS		:= $(BINDIR)/init.sys

KERNEL_ENTRY		:= kmain
KERNEL_OFFSET		:= 0x0000
KERNEL_SOURCES		:= kernel.S
KERNEL_OBJECTS		:= $(addprefix $(OBJDIR)/,$(KERNEL_SOURCES:.S=.o))
KERNEL_TARGETS		:= $(BINDIR)/kernel.sys

TARGETS 			:= $(STAGE1_TARGETS) $(STAGE2_TARGETS) $(KERNEL_TARGETS)

include $(TOPDIR)/Rules.mk

$(STAGE1_TARGETS): LDFLAGS := --oformat binary -Ttext $(STAGE1_OFFSET) -e $(STAGE1_ENTRY)
$(STAGE1_TARGETS): $(STAGE1_OBJECTS)

$(STAGE2_TARGETS): LDFLAGS := --oformat binary -Ttext $(STAGE2_OFFSET) -e $(STAGE2_ENTRY)
$(STAGE2_TARGETS): $(STAGE2_OBJECTS)

$(KERNEL_TARGETS): LDFLAGS := --oformat binary -Ttext $(KERNEL_OFFSET) -e $(KERNEL_ENTRY)
$(KERNEL_TARGETS): $(KERNEL_OBJECTS)
