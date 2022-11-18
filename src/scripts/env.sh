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
export _TOOLBIN=$_BINROOT/tools

export _MAKEROOT=$_OSROOT/Makefile
export _TOOLMAKEROOT=$_TOOLSRC/Makefile

echo "Setting build variables..."
echo "   _OSROOT = $_OSROOT"
echo "  _BINROOT = $_BINROOT"
echo "  _OBJROOT = $_OBJROOT"
echo "  _SRCROOT = $_SRCROOT"
echo "  _SCRIPTS = $_SCRIPTS"
echo "  _TOOLSRC = $_TOOLSRC"
echo "  _MAKEROOT = $_MAKEROOT"
echo "  _TOOLMAKEROOT = $_TOOLMAKEROOT"

qemu_path="/c/Program Files/qemu"    # TODO: this should go somewhere repository-local
binutils_path=$_OSROOT/lib/i686-elf-tools/bin

export PATH=$PATH:$qemu_path:$binutils_path:$_TOOLBIN:$_SCRIPTS

echo "Updating PATH..."
echo "     PATH += $qemu_path"
echo "     PATH += $binutils_path"
echo "     PATH += $_TOOLBIN"
echo "     PATH += $_SCRIPTS"

echo "All set!"
