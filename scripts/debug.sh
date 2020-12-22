#!/bin/bash
#==============================================================================#
# Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                    #
#                                                                              #
# This file is part of the OHWES Operating System.                             #
# OHWES is free software; you may redistribute it and/or modify it under the   #
# terms of the license agreement provided with this software.                  #
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
#                                                                              #
# QEMU debug attach.                                                           #
#==============================================================================#

if [ "$1" = "boot" ]; then
    gdb ${OHWES_BINDIR}/boot.elf \
        -ex 'target remote localhost:1234' \
        -ex 'set tdesc filename scripts/gdb_targets/i386-16bit.xml' \
        -ex 'set architecture i8086' \
        -ex 'break *0x7C00' \
        -ex 'layout asm' \
        -ex 'layout regs' \
        -ex 'continue'
elif [ "$1" = "init" ]; then
    gdb ${OHWES_BINDIR}/init.elf \
        -ex 'target remote localhost:1234' \
        -ex 'set tdesc filename scripts/gdb_targets/i386-16bit.xml' \
        -ex 'set architecture i8086' \
        -ex 'break *0x9000' \
        -ex 'layout src' \
        -ex 'layout regs' \
        -ex 'continue'
elif [ "$1" = "early_kernel" ]; then
    gdb ${OHWES_BINDIR}/ohwes.elf \
        -ex 'target remote localhost:1234' \
        -ex 'set architecture i386' \
        -ex 'break kentry' \
        -ex 'layout src' \
        -ex 'layout regs' \
        -ex 'continue'
else
    gdb ${OHWES_BINDIR}/ohwes.elf \
        -ex 'target remote localhost:1234' \
        -ex 'set architecture i386' \
        -ex 'break kmain' \
        -ex 'layout src' \
        -ex 'layout regs' \
        -ex 'continue'
fi
