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
#    File: tools/include/Rules.mk                                              #
# Created: Devember 12, 2020                                                   #
#  Author: Wes Hampson                                                         #
#==============================================================================#

.PHONY: all clean dirs

all: dirs $(BIN)

clean:
	@$(RM) -r $(BINDIR)
	@$(RM) -r $(OBJDIR)

dirs:
	@$(MKDIR) $(BINDIR)
	@$(MKDIR) $(OBJDIR)

$(BIN): $(OBJ)
	@echo 'LD  $(subst $(TOPDIR)/,,$^)'
	@$(LD) $(LDFLAGS) -o $@ $^
	@echo 'OUT $(subst $(TOPDIR)/,,$@)'

$(OBJDIR)/%.o: %.c
	@echo 'CC  $(join $(TREE),$<)'
	@$(CC) $(CFLAGS) -I$(INCLUDE) -MMD -c -o $@ $<

$(OBJDIR)/%.o: %.cpp
	@echo 'CXX $(join $(TREE),$<)'
	@$(CXX) $(CXXFLAGS) -I$(INCLUDE) -MMD -c -o $@ $<

$(OBJDIR)/%.o: %.S
	@echo 'AS  $(join $(TREE),$<)'
	@$(AS) $(ASFLAGS) -I$(INCLUDE) -c -o $@ $<

-include $(DEP)
