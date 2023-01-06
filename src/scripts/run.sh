#!/bin/bash
#===============================================================================
#    File: ohwes.sh
# Created: Jan 5, 2023
#  Author: Wes Hampson
#
# Boots OH-WES in QEMU.
#===============================================================================

qemu-system-i386.exe \
    -boot a \
    -fda $_BINROOT/ohwes.img
