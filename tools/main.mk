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
#         File: tools/main.mk
#      Created: October 2, 2023
#      Revised: November 4, 2026
#       Author: Wes Hampson
#
# boilermake system entrypoint for host tools build
# =============================================================================

ifndef TARGET_DIR
  $(error "tools must be built from the project root via `make tools`")
endif

TARGET_DIR := ${TARGET_DIR}/tools
BUILD_DIR  := ${BUILD_DIR}/tools

SUBMODULES := fatfs

# use host (native) toolchain
CC  := ${HOST_CC}
CXX := ${HOST_CXX}

CFLAGS := ${DEFAULT_CFLAGS}
CXXFLAGS := ${DEFAULT_CXXFLAGS}
