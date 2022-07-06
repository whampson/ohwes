#!/bin/bash
#===============================================================================
#    File: env.sh
# Created: June 27, 2022
#  Author: Wes Hampson
#
# Initializes the development environment by setting build variables and
# updating the system PATH.
# 
# To use, run `source env.sh` in your shell.
#===============================================================================

# !!! This file must live in <root>/src/scripts/ or _OSROOT will be defined incorrectly !!!

export _OSROOT=$(dirname $(dirname $(dirname $(realpath ${BASH_SOURCE[0]}))))
export _BINROOT=$_OSROOT/bin
export _OBJROOT=$_OSROOT/obj
export _SRCROOT=$_OSROOT/src
export _SCRIPTS=$_SRCROOT/scripts
export _TOOLSRC=$_SRCROOT/tools

export DEBUG=1

echo "Setting build variables..."
echo "   _OSROOT = $_OSROOT"
echo "  _BINROOT = $_BINROOT"
echo "  _OBJROOT = $_OBJROOT"
echo "  _SRCROOT = $_SRCROOT"
echo "  _SCRIPTS = $_SCRIPTS"

QEMU="/c/Program Files/qemu"    # TODO: this should go somewhere repository-local
BINUTILS=$_OSROOT/lib/i686-elf-tools/bin
TOOLSBIN=$_BINROOT/tools
export PATH=$PATH:$BINUTILS:$QEMU:$TOOLSBIN:$_SCRIPTS

echo "Updating PATH..."
echo "     PATH += $BINUTILS"
echo "     PATH += $QEMU"
echo "     PATH += $OSTOOLS"
echo "     PATH += $_SCRIPTS"

if [ -z "$DEBUG" ]; then
    echo "Debug build."
fi

echo "All set!"
