#!/bin/bash
#===============================================================================
#    File: ohwes.sh
# Created: Jan 5, 2023
#  Author: Wes Hampson
#
# Boots OH-WES in QEMU.
#===============================================================================

# Usage: run.sh bochs bochs_config
#        run.sh qemu disk_image

QEMU_PATH=/mingw32/bin/qemu-system-i386
BOCHS_PATH=/c/Program\ Files/Bochs-2.7/bochs.exe

if [ "$#" -lt 2 ]; then
    echo "Usage: run.sh bochs bochs_config"
    echo "       run.sh qemu disk_image"
    exit 1
fi

if [ "$1" = "qemu" ]; then
    QEMU_FLAGS=""
    QEMU_FLAGS+=" -m 32M"           # 32 MB of RAM
    QEMU_FLAGS+=" -boot a"          # boot drive A:
    QEMU_FLAGS+=" -fda $2"          # disk image

    if [ "$3" = "debug" ]; then
        QEMU_FLAGS+=" -S -s"
    fi

    echo "$QEMU_PATH" "$QEMU_FLAGS"
    "$QEMU_PATH" $QEMU_FLAGS
elif [ "$1" = "bochs" ]; then
    BOCHS_FLAGS=""
    BOCHS_FLAGS+=" -f $2"           # config file
    BOCHS_FLAGS+=" -q"              # no config menu

    echo $BOCHS_PATH $BOCHS_FLAGS
    "$BOCHS_PATH" $BOCHS_FLAGS
else
    echo "error: unknown emulator"
    exit 1
fi
