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
#    File: boot/Makefile                                                       #
# Created: November 22, 2020                                                   #
#  Author: Wes Hampson                                                         #
#==============================================================================#

ifeq ($(MAKELEVEL), 0)
$(error To build, please run 'make boot' from the top-level directory)
endif

include $(TOPDIR)/include/Tree.mk

STAGE1_ENTRY	:= entry
STAGE1_OFFSET	:= 0x7C00
STAGE1_SRC		:= fat.S
STAGE1_OBJ		:= $(addprefix $(OBJDIR)/,$(STAGE1_SRC:.S=.o))
STAGE1_BIN		:= $(BINDIR)/bootsect

STAGE2_ENTRY	:= init
STAGE2_OFFSET	:= 0x0000
STAGE2_SRC		:= init.S
STAGE2_OBJ		:= $(addprefix $(OBJDIR)/,$(STAGE2_SRC:.S=.o))
STAGE2_BIN		:= $(BINDIR)/init.sys

# Dummy, will be removed soon...
KERNEL_ENTRY	:= kmain
KERNEL_OFFSET	:= 0x0000
KERNEL_SRC		:= kernel.S
KERNEL_OBJ		:= $(addprefix $(OBJDIR)/,$(KERNEL_SRC:.S=.o))
KERNEL_BIN		:= $(BINDIR)/nbos.sys

BIN				:= $(STAGE1_BIN) $(STAGE2_BIN) $(KERNEL_BIN)

include $(TOPDIR)/include/Rules.mk

$(STAGE1_BIN): LDFLAGS := --oformat binary -Ttext $(STAGE1_OFFSET) -e $(STAGE1_ENTRY)
$(STAGE1_BIN): $(STAGE1_OBJ)

$(STAGE2_BIN): LDFLAGS := --oformat binary -Ttext $(STAGE2_OFFSET) -e $(STAGE2_ENTRY)
$(STAGE2_BIN): $(STAGE2_OBJ)

$(KERNEL_BIN): LDFLAGS := --oformat binary -Ttext $(KERNEL_OFFSET) -e $(KERNEL_ENTRY)
$(KERNEL_BIN): $(KERNEL_OBJ)
