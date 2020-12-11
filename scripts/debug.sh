#!/bin/bash
#==============================================================================#
# Copyright (C) 2020 Wes Hampson. All Rights Reserved.                         #
#                                                                              #
# This file is part of the Niobium Operating System.                           #
# Niobium is free software; you may redistribute it and/or modify it under     #
# the terms of the license agreement provided with this software.              #
#                                                                              #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL      #
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER   #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      #
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER          #
# DEALINGS IN THE SOFTWARE.                                                    #
#==============================================================================#
#    File: scripts/debug.sh                                                    #
# Created: November 22, 2020                                                   #
#  Author: Wes Hampson                                                         #
#------------------------------------------------------------------------------#
# QEMU debug attach.                                                           #
#==============================================================================#

if [ "$1" = "bootsect" ]; then
    gdb ${NB_OBJDIR}/boot/fat.o \
        -ex 'target remote localhost:1234' \
        -ex 'set tdesc filename scripts/gdb_targets/i386-16bit.xml' \
        -ex 'set architecture i8086' \
        -ex 'break *0x7C00' \
        -ex 'layout asm' \
        -ex 'layout regs' \
        -ex 'continue'
elif [ "$1" = "init" ]; then
    gdb ${NB_OBJDIR}/boot/init.o \
        -ex 'target remote localhost:1234' \
        -ex 'set tdesc filename scripts/gdb_targets/i386-16bit.xml' \
        -ex 'set architecture i8086' \
        -ex 'break *0x90000' \
        -ex 'layout src' \
        -ex 'layout regs' \
        -ex 'continue'
fi
