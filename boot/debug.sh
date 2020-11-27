#!/bin/bash

gdb -ex 'target remote localhost:1234' \
    -ex 'set architecture i8086' \
    -ex 'break *0x7c00' \
    -ex 'continue'
    -ex 'layout asm'
