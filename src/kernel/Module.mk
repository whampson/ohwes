# =============================================================================
# Copyright (C) 2023 Wes Hampson. All Rights Reserved.
#
# This file is part of the OH-WES Operating System.
# OH-WES is free software; you may redistribute it and/or modify it under the
# terms of the GNU GPLv2. See the LICENSE file in the root of this repository.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# -----------------------------------------------------------------------------
#         File: kernel/Module.mk
#      Created: March 27, 2023
#       Author: Wes Hampson
# =============================================================================

TARGETNAME = ohwes

# TODO: buildsys: appending this is leaky
INCLUDES += kernel/include

# NOTE: entry.S must be first!
SOURCES = \
	entry.S \
	console.c \
	debug.c \
	handler.c \
	init.c \
	interrupt.S \
	irq.c \

LINKLIBS = \
	$(LIB_ROOT)/sdk/libc/libc.a \

 # Code Origin
LDFLAGS += -Ttext 0x100000		# TODO: LEAKY!!!

# $(eval $(call make-lib, lib$(TARGETNAME).a, $(SOURCES)))
$(eval $(call make-exe, $(TARGETNAME).elf, $(SOURCES), $(LINKLIBS)))

# Strip .elf to create .sys
$(BIN_ROOT)/$(TARGETNAME).sys: $(BIN_ROOT)/$(TARGETNAME).elf
	objcopy -Obinary $< $@

# Make sure buildsys knows about .sys
# TODO: this is kind of a hack
_BINARIES += \
	$(BIN_ROOT)/$(TARGETNAME).sys \
