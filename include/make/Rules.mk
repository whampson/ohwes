#==============================================================================#
# Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                    #
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
#    File: include/make/Rules.mk                                               #
# Created: November 27, 2020                                                   #
#  Author: Wes Hampson                                                         #
#                                                                              #
# Makefile rules common to all parts of the build.                             #
#==============================================================================#

.PHONY: all clean dirs
.PRECIOUS: $(BINDIR)/%.elf

all: dirs $(BIN)

clean:
	@$(RM) -r $(BINDIR)
	@$(RM) -r $(OBJDIR)

dirs:
	@$(MKDIR) $(BINDIR)
	@$(MKDIR) $(OBJDIR)

$(BINDIR)/%.bin: $(BINDIR)/%.elf
	@$(OBJCOPY) -O binary $< $@
	@echo 'OUT $(subst $(TOPDIR)/,,$(abspath $@))'

$(BINDIR)/%.sys: $(BINDIR)/%.elf
	@$(OBJCOPY) -O binary $< $@
	@echo 'OUT $(subst $(TOPDIR)/,,$(abspath $@))'

$(BINDIR)/%.elf: $(OBJ)
	@echo 'LD  $(LDFLAGS) $(subst $(TOPDIR)/,,$(realpath $^))'
	@$(LD) $(LDFLAGS) -o $@ $^

$(OBJDIR)/%.o: %.c
	@echo 'CC  $(CFLAGS) $(TREE)$<'
	@$(CC) $(CFLAGS) -MD -MF $(@:.o=.d) -I$(INCLUDE) -c -o $@ $<

$(OBJDIR)/%.o: %.S
	@echo 'AS  $(ASFLAGS) $(TREE)$<'
	@$(AS) $(ASFLAGS) -MD -MF $(@:.o=.d) -I$(INCLUDE) -c -o $@ $<

-include $(DEP)
