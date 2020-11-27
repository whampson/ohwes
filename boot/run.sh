#==============================================================================#
# Copyright (C) 2020 Wes Hampson. All Rights Reserved.                         #
#                                                                              #
# This file is part of the Niobium Operating System.                           #
# Niobium is free software; you may redistribute it and/or modify it under     #
# the terms of the license agreement provided with this software.              #
#==============================================================================#

#!/bin/bash

qemu_args="-drive file=floppy.img,if=floppy,format=raw,index=0 -boot a -m 2G"

if [ "$1" = "d" ] || [ "$1" = "debug" ]; then
    qemu_args="-s -S $qemu_args"
fi

qemu-system-i386 $qemu_args
