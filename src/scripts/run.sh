#!/bin/bash
#===============================================================================
#    File: ohwes.sh
# Created: Jan 5, 2023
#  Author: Wes Hampson
#
# Boots OH-WES in QEMU.
#===============================================================================

QEMU_FLAGS=""
QEMU_FLAGS+=" -boot a"
QEMU_FLAGS+=" -fda $_BINROOT/ohwes.img"

if [ "$1" = "debug" ]; then
QEMU_FLAGS+=" -S -m 16 -s"
fi

QEMU_EXEC="qemu-system-i386w.exe $QEMU_FLAGS"

echo $QEMU_EXEC
$QEMU_EXEC
