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

# !!! This file must live in <root>/scripts/ or _OSROOT will be defined incorrectly !!!

export _OSROOT=$(dirname $(dirname $(realpath ${BASH_SOURCE[0]})))
export _BINROOT=$_OSROOT/bin
export _OBJROOT=$_OSROOT/obj
export _SRCROOT=$_OSROOT/src
export _SCRIPTS=$_OSROOT/scripts
export _TOOLSRC=$_OSROOT/tools

# TODO: set these based on script args
export _DEBUG=1

QEMU="/c/Program Files/qemu"    # TODO: this should go somewhere local
BINUTILS=$_OSROOT/lib/i686-elf-tools/bin
OSTOOLS=$_TOOLSRC/bin
export PATH=$PATH:$BINUTILS:$QEMU:$OSTOOLS:$_SCRIPTS

echo "Setting build variables..."
echo "   _OSROOT = $_OSROOT"
echo "  _BINROOT = $_BINROOT"
echo "  _OBJROOT = $_OBJROOT"
echo "  _SRCROOT = $_SRCROOT"
echo "  _SCRIPTS = $_SCRIPTS"
echo "  _TOOLSRC = $_TOOLSRC"
echo "    _DEBUG = $_DEBUG"
echo 
echo "Updating PATH..."
echo "  PATH += $BINUTILS"
echo "  PATH += $QEMU"
echo "  PATH += $OSTOOLS"
echo "  PATH += $_SCRIPTS"

echo "All set!"
