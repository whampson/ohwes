#!/bin/bash
#===============================================================================
#    File: ohwes.sh
# Created: Jan 5, 2023
#  Author: Wes Hampson
#
# Boots OH-WES in QEMU.
#===============================================================================

# Usage: run.sh diskimg
# diskimg - floppy disk image

if [ "$1" = "" ]; then
echo "error: missing diskimg argument"
exit 1
fi

QEMU_FLAGS=""
QEMU_FLAGS+=" -m 4M"
QEMU_FLAGS+=" -boot a"
QEMU_FLAGS+=" -fda $1"

if [ "$2" = "debug" ]; then
QEMU_FLAGS+=" -S -s"
fi

QEMU_EXEC="qemu-system-i386w.exe $QEMU_FLAGS"

echo $QEMU_EXEC
$QEMU_EXEC
