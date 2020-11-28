#!/bin/bash
#==============================================================================#
# Copyright (C) 2020 Wes Hampson. All Rights Reserved.                         #
#                                                                              #
# This file is part of the Niobium Operating System.                           #
# Niobium is free software; you may redistribute it and/or modify it under     #
# the terms of the license agreement provided with this software.              #
#==============================================================================#

qemu_args+=" -boot a "
qemu_args+=" -m 2G"
qemu_args+=" -drive file=${NBDIR}/floppy.img,if=floppy,format=raw,index=0"

if [ "$1" = "d" ] || [ "$1" = "debug" ]; then
    qemu_args+=" -s -S"
fi

qemu-system-i386 $qemu_args
