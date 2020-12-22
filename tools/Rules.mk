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
#    File: tools/include/Rules.mk                                              #
# Created: Devember 12, 2020                                                   #
#  Author: Wes Hampson                                                         #
#==============================================================================#

include $(TOPDIR)/Rules.mk

$(BIN): $(OBJ)
	@echo 'LD  $(LDFLAGS) $(subst $(TOPDIR)/,,$(realpath $^))'
	@$(LD) $(LDFLAGS) -o $@ $^
	@echo 'OUT $(subst $(TOPDIR)/,,$(abspath $@))'

$(OBJDIR)/%.o: %.cpp
	@echo 'CXX $(CXXFLAGS) $(TREE)$<'
	@$(CXX) $(CXXFLAGS) -MMD -MF $(@:.o=.d) -I$(INCLUDE) -c -o $@ $<
