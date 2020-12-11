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
#    File: tools/fatfs/Makefile                                                #
# Created: November 27, 2020                                                   #
#  Author: Wes Hampson                                                         #
#==============================================================================#

ifeq ($(MAKELEVEL), 0)
$(error To build, please run 'make' from the top-level directory)
endif

include $(TOPDIR)/include/Tree.mk

SRC_S	:= $(wildcard *.S)
SRC_C	:= $(wildcard *.c)

OBJ	:= 	$(addprefix $(OBJDIR)/,$(SRC_S:.S=.o)) \
		$(addprefix $(OBJDIR)/,$(SRC_C:.c=.o)) \
		$(wildcard $(OBJDIR)/fs/*.o)
DEP	:= 	$(OBJ:%.o=%.d)
BIN := 	$(BINDIR)/nbos.sys

include $(TOPDIR)/include/Rules.mk

$(BIN): LDFLAGS := --oformat binary -T kernel.ld
$(BIN): $(OBJ)