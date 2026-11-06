#!/bin/bash
#===============================================================================
#    File: scripts/def.sh
# Created: October 25, 2025
#  Author: Wes Hampson
#
# Environment defaults. Overridable environment variables are defined here.
# Useful for sharing defaults between scripts and Makefiles. Set a variable
# before sourcing this script to override.
#===============================================================================

export HOST_CC=${HOST_CC:-gcc}
export HOST_CXX=${HOST_CXX:-g++}

export TOOLCHAIN=${TOOLCHAIN:-i686-elf}
export CROSS_CC=${TOOLCHAIN}-gcc
export CROSS_AR=${TOOLCHAIN}-ar
export CROSS_AS=${TOOLCHAIN}-gcc
export CROSS_CXX=${TOOLCHAIN}-g++
export CROSS_LD=${TOOLCHAIN}-gcc
export CROSS_OBJCOPY=${TOOLCHAIN}-objcopy

export QEMU=${QEMU:-qemu-system-i386}
export BOCHS=${BOCHS:-bochs}
export GDB=${GDB:-gdb}
