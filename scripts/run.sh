#!/bin/bash
#===============================================================================
#    File: ohwes.sh
# Created: Jan 5, 2023
#  Author: Wes Hampson
#
# Boots OH-WES in QEMU.
#===============================================================================

# Usage: run.sh emu img
# emu - emulator (qemu, bochs)
# img - floppy disk image path

QEMU_PATH=/mingw32/bin/qemu-system-i386
BOCHS_PATH=/c/Program\ Files/Bochs-2.7/bochs.exe

if [ "$1" = "" ]; then
    echo "error: missing emulator argument"
    exit 1
fi

if [ "$2" = "" ]; then
    echo "error: missing disk image argument"
    exit 1
fi

if [ "$1" = "qemu" ]; then
    QEMU_FLAGS=""
    QEMU_FLAGS+=" -m 4M"            # 4 MB of RAM
    QEMU_FLAGS+=" -boot a"          # boot drive A:
    QEMU_FLAGS+=" -fda $2"          # disk image

    if [ "$3" = "debug" ]; then
        QEMU_FLAGS+=" -S -s"
    fi

    echo "$QEMU_PATH" "$QEMU_FLAGS"
    "$QEMU_PATH" $QEMU_FLAGS
elif [ "$1" = "bochs" ]; then
    BOCHS_FLAGS=""
    BOCHS_FLAGS+=" -f bochsrc.bxrc" # config file
    BOCHS_FLAGS+=" -q"              # no config menu

    echo $BOCHS_PATH $BOCHS_FLAGS
    "$BOCHS_PATH" $BOCHS_FLAGS
else
    echo "error: unknown emulator"
    exit 1
fi
