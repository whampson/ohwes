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
#         File: boot/Module.mk
#      Created: Mar 12, 2023
#       Author: Wes Hampson
# =============================================================================

TARGET  = boot.elf
SOURCES = \
	stage1.S \
	stage2.S \
	init.c \

LINKLIBS = \
	$(LIB_ROOT)/kernel/libkernel.a \

 # ORIGIN
LDFLAGS += -Ttext 0x7C00

$(eval $(call make-exe, $(TARGET), $(SOURCES), $(LINKLIBS)))

# split boot.elf into boot.bin, bootsect.bin and boot.sys
#   boot.bin = stripped boot.elf
#   bootsect.bin = bytes 0-511 of boot.bin (boot sector)
#   boot.sys = bytes 512-end of boot.bin

_TARGETS += \
	$(BIN_ROOT)/boot.bin \
	$(BIN_ROOT)/boot.sys \
	$(BIN_ROOT)/bootsect.bin

# strip
$(BIN_ROOT)/boot.bin: $(BIN_ROOT)/boot.elf
	objcopy -Obinary $< $@

# extract boot sector
$(BIN_ROOT)/bootsect.bin: $(BIN_ROOT)/boot.bin
	head -c 512 $< > $@

# rest of boot loader
$(BIN_ROOT)/boot.sys: $(BIN_ROOT)/boot.bin
	tail -c +512 $< > $@