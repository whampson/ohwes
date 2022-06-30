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
#    File: tools/fatfs/Makefile                                                #
# Created: November 27, 2020                                                   #
#  Author: Wes Hampson                                                         #
#==============================================================================#

BIN = $(BINDIR)/fatfs

include $(_TOOLSRC)/Common.mk

SRC = $(wildcard *.cpp)
OBJ = $(addprefix $(OBJDIR)/,$(SRC:.cpp=.o))

$(BIN): $(OBJ)
	@echo 'LD      $(LDFLAGS) $^'
	@$(LD) $(LDFLAGS) -o $@ $^

$(OBJDIR)/%.o: %.cpp
	@echo 'CXX     $(CXXFLAGS) $(TREE)/$<'
	@$(CXX) $(CXXFLAGS) $(addprefix -I,$(INC)) -MMD -MF $(@:.o=.d)  -c -o $@ $<
