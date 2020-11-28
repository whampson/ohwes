#==============================================================================#
# Copyright (C) 2020 Wes Hampson. All Rights Reserved.                         #
#                                                                              #
# This file is part of the Niobium Operating System.                           #
# Niobium is free software; you may redistribute it and/or modify it under     #
# the terms of the license agreement provided with this software.              #
#==============================================================================#

# Directory tracking for recursive build

export DIRNAME 		:= $(notdir $(CURDIR))
export TREE			:= $(TREE)$(DIRNAME)/
export OBJDIR		:= $(OBJDIR)/$(DIRNAME)
