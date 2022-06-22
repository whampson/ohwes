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
#    File: boot/Makefile                                                       #
# Created: November 22, 2020                                                   #
#  Author: Wes Hampson                                                         #
#==============================================================================#

ifeq ($(MAKELEVEL), 0)
$(error To build, please run 'make boot' from the top-level directory)
endif

include $(TOPDIR)/Tree.mk

STAGE1_ENTRY	:= entry
STAGE1_OFFSET	:= 0x7C00
STAGE1_SRC	:= fat.S
STAGE1_OBJ	:= $(addprefix $(OBJDIR)/,$(STAGE1_SRC:.S=.o))
STAGE1_BIN	:= $(BINDIR)/boot.bin

STAGE2_ENTRY	:= init
STAGE2_OFFSET	:= 0x0000
STAGE2_SRC	:= init.S
STAGE2_OBJ	:= $(addprefix $(OBJDIR)/,$(STAGE2_SRC:.S=.o))
STAGE2_BIN	:= $(BINDIR)/init.sys

BIN		:= $(STAGE1_BIN) $(STAGE2_BIN)

include $(TOPDIR)/Rules.mk

$(BINDIR)/boot.elf: LDFLAGS := -Ttext $(STAGE1_OFFSET) -e $(STAGE1_ENTRY)
$(BINDIR)/boot.elf: $(STAGE1_OBJ)

$(BINDIR)/init.elf: LDFLAGS := -Ttext $(STAGE2_OFFSET) -e $(STAGE2_ENTRY)
$(BINDIR)/init.elf: $(STAGE2_OBJ)
