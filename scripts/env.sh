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

PROJ_ROOT=$(realpath $(dirname $BASH_SOURCE[0])/..)

CC=i686-elf-gcc
QEMU_i386=qemu-system-i386
MAKE=make

CC_NATIVE=gcc
CXX_NATIVE=g++

if [[ "$OSTYPE" == "msys" ]]; then          # Windows/MINGW32
    QEMU_PATH="/c/Program Files/qemu"
    if [ -d "$QEMU_PATH" ]; then
        PATH=$PATH:$QEMU_PATH
    fi

    ELF_TOOLS="$PROJ_ROOT/build/i686-elf-tools/bin"
    if [ -d "$ELF_TOOLS" ]; then
        PATH=$PATH:$ELF_TOOLS
    fi
else
    # TODO: darwin, linux
    echo "error: unsupported platform '$OSTYPE'!"
    return 1
fi

TOOLS_PATH=$PROJ_ROOT/tools/bin
PATH=$PATH:$TOOLS_PATH

echo "$MAKE => $(command -v $MAKE)"
echo "$CC_NATIVE => $(command -v $CC_NATIVE)"
echo "$CXX_NATIVE => $(command -v $CXX_NATIVE)"
echo "$CC => $(command -v $CC)"
echo "$QEMU_i386 => $(command -v $QEMU_i386)"
echo ""

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

# Makefile Autocompletion
# https://stackoverflow.com/a/38415982
complete -W "\`grep -oE '^[a-zA-Z0-9_.-]+:([^=]|$)' ?akefile | sed 's/[^a-zA-Z0-9_.-]*$//'\`" make

export OHWES_ENVIRONMENT_SET=1
echo "All set!"
