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

TARGETNAME := kernel
BINNAME    := ohwes

SOURCES := \
    console.c \
    debug.c \
    entry.S \
    handler.c \
    init.c \
    interrupt.S \
    irq.c \

LNKLIBS := \
    $(OBJ_ROOT)/crt/crt.lib

CFLAGS  := -I$(SRC)/include
ASFLAGS := -I$(SRC)/include
LDFLAGS := -Ttext 0x100000

$(eval $(call COMPILE,$(TARGETNAME),$(SOURCES),$(CFLAGS),$(ASFLAGS)))
$(eval $(call LINK,$(TARGETNAME),$(LNKLIBS),$(LDFLAGS)))
$(eval $(call STRIP,$(TARGETNAME)))
$(eval $(call BINPLACE,$(BINNAME).sys,$(OBJ)/$(TARGETNAME).bin))
