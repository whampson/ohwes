#!/bin/bash

qemu_args="-drive file=floppy.img,if=floppy,format=raw,index=0 -boot a -m 2G"

if [ "$1" = "d" ] || [ "$1" = "debug" ]; then
    qemu_args="-s -S $qemu_args"
fi

qemu-system-i386 $qemu_args
