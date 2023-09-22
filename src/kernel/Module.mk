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

SOURCES = \
	console.c \
	debug.c \
	entry.S \
	handler.c \
	init.c \
	interrupt.S \
	irq.c \

LINKLIBS = obj/crt/crt.lib

CFLAGS  := -Isrc/kernel/include
ASFLAGS := -Isrc/kernel/include
LDFLAGS := -Ttext 0x100000

# $(eval $(call make-lib,$(TARGETNAME).elf,$(SOURCES),$(CFLAGS),$(ASFLAGS)))
$(eval $(call make-exe,$(TARGETNAME).elf,$(SOURCES),$(LINKLIBS),$(CFLAGS),$(ASFLAGS),$(LDFLAGS)))
$(eval $(call make-sys,$(TARGETNAME).sys,$(TARGETNAME).elf))

