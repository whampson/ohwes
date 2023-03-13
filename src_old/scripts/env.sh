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

# !!! This file must live in /src/scripts/ !!!
# !!! Otherwise, OSROOT will be defined incorrectly !!!

export OSROOT=$(dirname $(dirname $(dirname $(realpath ${BASH_SOURCE[0]}))))

export MAKEROOT=$OSROOT/Makefile
export BINROOT=$OSROOT/bin
export OBJROOT=$OSROOT/obj
export SRCROOT=$OSROOT/src
export SCRIPTS=$SRCROOT/scripts
export TOOLSBIN=$BINROOT/tools
export TOOLSSRC=$SRCROOT/tools

echo "Setting build variables..."
echo "  OSROOT = $OSROOT"
echo "MAKEROOT = $MAKEROOT"
echo " BINROOT = $BINROOT"
echo " OBJROOT = $OBJROOT"
echo " SRCROOT = $SRCROOT"
echo " SCRIPTS = $SCRIPTS"
echo "TOOLSBIN = $TOOLSBIN"
echo "TOOLSSRC = $TOOLSSRC"

binutils_path=$OSROOT/lib/i686-elf-tools/bin

export PATH=$PATH:$binutils_path:$TOOLSBIN:$SCRIPTS

echo "Updating PATH..."
echo "     PATH += $binutils_path"
echo "     PATH += $TOOLSBIN"
echo "     PATH += $SCRIPTS"

echo "All set!"
