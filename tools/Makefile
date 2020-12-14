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
#    File: tools/Makefile                                                      #
# Created: November 27, 2020                                                   #
#  Author: Wes Hampson                                                         #
#==============================================================================#

ifeq ($(MAKELEVEL), 0)
$(error To build, please run 'make tools' from the top-level directory)
endif

include $(TOPDIR)/include/make/Tree.mk

export TOOLSDIR		:= $(CURDIR)
export OBJDIR		:= obj
export BINDIR		:= bin
export INCLUDE		:= include

export CXX		:= g++
export CXXFLAGS		:= -g
export LD		:= g++
export LDFLAGS		:=

.PHONY: all fatfs

all: fatfs

fatfs: dirs
	@$(MAKE) -C fatfs	

include $(TOOLSDIR)/include/make/Rules.mk
