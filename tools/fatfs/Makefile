#==============================================================================#
# Copyright (C) 2020 Wes Hampson. All Rights Reserved.                         #
#                                                                              #
# This file is part of the Niobium Operating System.                           #
# Niobium is free software; you may redistribute it and/or modify it under the #
# terms of the license agreement provided with this software.                  #
#==============================================================================#

ifeq ($(MAKELEVEL), 0)
$(error To build, please run 'make tools' from the top-level directory)
endif

include $(TOPDIR)/include/Tree.mk

SOURCES				:= $(wildcard *.c)
OBJECTS 			:= $(addprefix $(OBJDIR)/,$(SOURCES:.c=.o))
TARGETS				:= $(BINDIR)/fatfs

include $(TOPDIR)/include/Rules.mk
