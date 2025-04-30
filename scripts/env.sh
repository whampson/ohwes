#!/bin/bash
#===============================================================================
#    File: scripts/env.sh
# Created: September 23, 2023
#  Author: Wes Hampson
#
# Sets up the environment for building OH-WES.
#===============================================================================

if [ ! -z "$OHWES_ENVIRONMENT_SET" ]; then
    echo "warning: OH-WES environment already set!"
    echo ""
fi

# Get project root path (relative to this script)
PROJ_ROOT=$(realpath $(dirname $BASH_SOURCE[0])/..)

# Makefile Autocompletion
# https://stackoverflow.com/a/38415982
complete -W "\`grep -oE '^[a-zA-Z0-9_.-]+:([^=]|$)' main.mk | sed -r 's/[^a-zA-Z0-9_.-]*$//'\`" make
# TOOD: make this work with all .mk files, not just main.mk

# Executables
CC=i686-elf-gcc
QEMU_i386=qemu-system-i386
MAKE=make
CC_NATIVE=gcc
CXX_NATIVE=g++

# Platform-specific stuff
if [[ "$(expr substr $(uname -s) 1 10)" == "MINGW32_NT" ]]; then # Windows/MINGW32
    QEMU_PATH="/c/Program Files/qemu"
    if [ -d "$QEMU_PATH" ]; then
        PATH=$PATH:$QEMU_PATH
    fi
# else
#     # TODO: darwin, linux
fi

# Add i686-elf-tools to PATH
ELF_TOOLS="$PROJ_ROOT/build/i686-elf-tools/bin"
if [ -d "$ELF_TOOLS" ]; then
    PATH=$PATH:$ELF_TOOLS
else
    echo "error: i868-elf-tools binaries not found!"
fi

# Add native-built tools to PATH
TOOLS_PATH=$PROJ_ROOT/bin/tools
PATH=$PATH:$TOOLS_PATH

# Print executable paths for sanity
echo "$MAKE => $(command -v $MAKE)"
echo "$CC_NATIVE => $(command -v $CC_NATIVE)"
echo "$CXX_NATIVE => $(command -v $CXX_NATIVE)"
echo "$CC => $(command -v $CC)"
echo "$QEMU_i386 => $(command -v $QEMU_i386)"
echo ""

# Checks
if [[ -z "$(command -v $CC_NATIVE)" ]]; then
    echo "error: missing native gcc!"
    return 1
fi
if [[ -z "$(command -v $CXX_NATIVE)" ]]; then
    echo "error: missing native g++!"
    return 1
fi
if [[ -z "$(command -v $CC)" ]]; then
    echo "error: missing g++!"
    return 1
fi
if [[ -z "$(command -v $MAKE)" ]]; then
    echo "error: missing GNU Make!"
    return 1
fi
if [[ -z "$(command -v $QEMU_i386)" ]]; then
    echo "warning: qemu not found!"
fi

# All done!
export OHWES_ENVIRONMENT_SET=1
echo "All set!"
