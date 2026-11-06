#==============================================================================
# Copyright (C) 2020-2026 Wes Hampson. All Rights Reserved.
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
#         File: Module.mk
#      Created: September 29, 2023
#      Revised: November 4, 2026
#       Author: Wes Hampson
#
# OH-WES kernel build configuration
# =============================================================================

# target architecture
ARCH        := i386
ifeq "${ARCH}" "i386"
  MARCH     := pentium
endif

# include directory (relative to repo root)
INCLUDES := src/include

# OS modules to build
SUBMODULES := \
    ${ARCH}/boot \
    ${ARCH}/kernel \
    kernel libc usr

# use cross compiler toolchain
AR         := ${CROSS_AR}
AS         := ${CROSS_AS}
CC         := ${CROSS_CC}
CXX        := ${CROSS_CXX}
LD         := ${CROSS_LD}
OBJCOPY    := ${CROSS_OBJCOPY}

# global OH-WES build flags
ARFLAGS := ${DEFAULT_ARFLAGS} -rcs
ASFLAGS := ${DEFAULT_ASFLAGS} -include _config.h
LDFLAGS := ${DEFAULT_LDFLAGS} -nostdlib -lgcc
CFLAGS  := ${DEFAULT_CFLAGS} -include ${ARCH}/compiler.h -include _config.h
CFLAGS  += -std=c11 -march=${MARCH} -nostdinc -ffreestanding
CFLAGS  += -fno-unwind-tables -fno-asynchronous-unwind-tables
CFLAGS  += -fno-exceptions
