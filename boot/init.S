#==============================================================================#
# Copyright (C) 2020 Wes Hampson. All Rights Reserved.                         #
#                                                                              #
# This file is part of the Niobium Operating System.                           #
# Niobium is free software; you may redistribute it and/or modify it under     #
# the terms of the license agreement provided with this software.              #
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
#     File: init.S                                                             #
#   Author: Wes Hampson                                                        #
#     Date: Nov. 25, 2020                                                      #
#     Desc: (TODO)                                                             #
#==============================================================================#

.code16

.global init
init:
    leaw    stage2_msg0, %si
    call    print
    leaw    stage2_msg1, %si
    call    print
    leaw    stage2_msg2, %si
    call    print

enter_kernel:
    ljmp    $0x1000, $0x0000

####
# Prints a string.
#
#   Inputs: si - address of string
#  Outputs: (none)
# Clobbers: ax, bx, si
####
print:
    movb    $0x0E, %ah      # 10h Mode: Write Character as TTY
    movw    $0x07, %bx      # Video Attribute: bg = black, fg = gray
_print_loop:
    lodsb
    andb    %al, %al
    jz      _print_done
    int     $0x10
    jmp     _print_loop
_print_done:
    ret

# Data spans multiple disk clusters
stage2_msg0:        .asciz      "You made it to stage 2!\r\n"
pad0:               .space      1024, 0xA5
stage2_msg1:        .asciz      "You found the secret message!\r\n"
pad1:               .space      1024, 0x5A
stage2_msg2:        .asciz      "Wow, you found another secret message!\r\n"
