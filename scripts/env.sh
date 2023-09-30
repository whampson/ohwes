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
fi

PROJ_ROOT=$(realpath $(dirname $BASH_SOURCE[0])/..)

GCC_CMD_NATIVE=gcc
GCC_CMD_OHWES=i686-elf-gcc
QEMU_CMD=qemu-system-i386
MAKE_CMD=make

if [[ "$OSTYPE" == "msys" ]]; then          # Windows/MINGW32
    QEMU_PATH="/c/Program Files/qemu"
    if [ -d "$QEMU_PATH" ]; then
        PATH=$PATH:$QEMU_PATH
    fi

    ELF_TOOLS="$PROJ_ROOT/tools/bin/i686-elf-tools-windows/bin"
    if [ -d "$ELF_TOOLS" ]; then
        PATH=$PATH:$ELF_TOOLS
    fi
else
    # TODO: darwin, linux
    echo "error: unsupported platform '$OSTYPE'!"
    return 1
fi

echo "$MAKE_CMD => $(command -v $MAKE_CMD)"
echo "$GCC_CMD_NATIVE => $(command -v $GCC_CMD_NATIVE)"
echo "$GCC_CMD_OHWES => $(command -v $GCC_CMD_OHWES)"
echo "$QEMU_CMD => $(command -v $QEMU_CMD)"
echo ""

if [[ -z "$(command -v $GCC_CMD_NATIVE)" ]]; then
    echo "error: missing GCC native compiler!"
    return 1
fi
if [[ -z "$(command -v $GCC_CMD_OHWES)" ]]; then
    echo "error: missing GCC cross-compiler!"
    return 1
fi
if [[ -z "$(command -v $MAKE_CMD)" ]]; then
    echo "error: missing GNU Make!"
    return 1
fi
if [[ -z "$(command -v $QEMU_CMD)" ]]; then
    echo "warning: QEMU_CMD not found!"
fi

# Makefile Autocompletion
# https://stackoverflow.com/a/38415982
complete -W "\`grep -oE '^[a-zA-Z0-9_.-]+:([^=]|$)' ?akefile | sed 's/[^a-zA-Z0-9_.-]*$//'\`" make

export OHWES_ENVIRONMENT_SET=1
echo "All set!"
