#==============================================================================#
# Copyright (C) 2020 Wes Hampson. All Rights Reserved.                         #
#                                                                              #
# This file is part of the Niobium Operating System.                           #
# Niobium is free software; you may redistribute it and/or modify it under     #
# the terms of the license agreement provided with this software.              #
#==============================================================================#

# Makefile rules common to all parts of the build

.PHONY: all clean dirs

all: dirs $(TARGETS)

clean:
	@$(RM) -r $(BINDIR)
	@$(RM) -r $(OBJDIR)

dirs:
	@$(MKDIR) $(BINDIR)
	@$(MKDIR) $(OBJDIR)

$(TARGETS): $(OBJECTS)
	@echo 'LD  $(subst $(TOPDIR)/,,$^)'
	@$(LD) $(LDFLAGS) -o $@ $^
	@echo 'OUT $(subst $(TOPDIR)/,,$@)'

$(OBJDIR)/%.o: %.c
	@echo 'CC  $(join $(TREE),$^)'
	@$(CC) $(CFLAGS) -c -o $@ $^

$(OBJDIR)/%.o: %.cpp
	@echo 'CXX $(join $(TREE),$^)'
	@$(CXX) $(CXXFLAGS) -c -o $@ $^

$(OBJDIR)/%.o: %.S
	@echo 'AS  $(join $(TREE),$^)'
	@$(AS) $(ASFLAGS) -c -o $@ $^
