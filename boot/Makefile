#==============================================================================#
# Copyright (C) 2020 Wes Hampson. All Rights Reserved.                         #
#                                                                              #
# This file is part of the Niobium Operating System.                           #
# Niobium is free software; you may redistribute it and/or modify it under     #
# the terms of the license agreement provided with this software.              #
#==============================================================================#

ifneq ($(MAKELEVEL), 1)
$(error To build, please run 'make boot' from the top-level directory)
endif

TARGETS				:= bootsect init.sys kernel.sys

CURDIR_R			:= $(subst $(BASEDIR)/,,$(CURDIR))
BINDIR_R			:= $(subst $(BASEDIR)/,,$(BINDIR))
OBJDIR_R			:= $(subst $(BASEDIR)/,,$(OBJDIR)/$(CURDIR_R))
OBJDIR				:= $(OBJDIR)/$(CURDIR_R)

STAGE1_SOURCES		:= fat.S
STAGE1_OBJECTS		= $(addprefix $(OBJDIR)/,$(STAGE1_SOURCES:.S=.o))
STAGE1_ENTRY		:= entry
STAGE1_OFFSET		:= 0x7C00

STAGE2_SOURCES		:= init.S
STAGE2_OBJECTS		= $(addprefix $(OBJDIR)/,$(STAGE2_SOURCES:.S=.o))
STAGE2_ENTRY		:= init
STAGE2_OFFSET		:= 0x8000

KERNEL_SOURCES		:= kernel.S
KERNEL_OBJECTS		= $(addprefix $(OBJDIR)/,$(KERNEL_SOURCES:.S=.o))
KERNEL_ENTRY		:= kmain
KERNEL_OFFSET		:= 0x0000

.PHONY: all dirs

all: dirs $(addprefix $(BINDIR)/,$(TARGETS))

dirs:
	@$(MKDIR) $(BINDIR)
	@$(MKDIR) $(OBJDIR)

$(BINDIR)/bootsect: $(STAGE1_OBJECTS)
	@printf 'LD  $(OBJDIR_R)/$(notdir $^)\n==> $(BINDIR_R)/$(notdir $@)\n'
	@$(LD) --oformat binary -Ttext $(STAGE1_OFFSET) -e $(STAGE1_ENTRY) -o $@ $^

$(BINDIR)/init.sys: $(STAGE2_OBJECTS)
	@printf 'LD  $(OBJDIR_R)/$(notdir $^)\n==> $(BINDIR_R)/$(notdir $@)\n'
	@$(LD) --oformat binary -Ttext $(STAGE2_OFFSET) -e $(STAGE2_ENTRY) -o $@ $^

$(BINDIR)/kernel.sys: $(KERNEL_OBJECTS)
	@printf 'LD  $(OBJDIR_R)/$(notdir $^)\n==> $(BINDIR_R)/$(notdir $@)\n'
	@$(LD) --oformat binary -Ttext $(KERNEL_OFFSET) -e $(KERNEL_ENTRY) -o $@ $^

$(OBJDIR)/%.o: %.S
	@echo 'AS  $(CURDIR_R)/$<'
	@$(AS) $(ASFLAGS) -c -o $@ $<
