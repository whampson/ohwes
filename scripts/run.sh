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
#    File: scripts/ruh.sh                                                      #
# Created: November 22, 2020                                                   #
#  Author: Wes Hampson                                                         #
#                                                                              #
# Boots OHWES in QEMU.                                                         #
#==============================================================================#

if [ -z ${OHWES_BINDIR+x} ]; then
    echo "Error: OHWES develoment environment not set."
    echo "Please source 'scripts/devenv.sh' and try again."
    exit 1
fi

qemu="qemu-system-i386"
qemu_args+=" -boot a"
qemu_args+=" -m 128M"
qemu_args+=" -drive file=${OHWES_BINDIR}/ohwes.img,if=floppy,format=raw,index=0"
qemu_args+=" -serial file:CON"
if [ "$1" = "d" ] || [ "$1" = "debug" ]; then
    qemu_args+=" -s -S"
fi

$qemu $qemu_args
