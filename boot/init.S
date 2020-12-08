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
#    File: boot/init.S                                                         #
# Created: November 25, 2020                                                   #
#  Author: Wes Hampson                                                         #
#==============================================================================#

.code16

.global init
init:
    pushw   %cs
    popw    %ds
//     clc
// get_memory:
//     xorw    %cx, %cx
//     xorw    %dx, %dx
//     movw    $0xE801, %ax
//     int     $0x15
//     cmpb    $0x86, %ah      # unsupported function
//     je      error
//     cmpb    $0x88, %ah      # invalid function
//     je      error
//     jcxz    get_memory_result
//     movw    %cx, %ax
//     movw    %dx, %bx

// get_memory_result:
//     # ax = number of contiguous KiB, 1M to 16M
//     # bx = number of contiguous 64 MiB pages above 16M

    leaw    happy_msg, %si
    call    print
    jmp     enter_kernel

error:
    leaw    error_msg, %si
    call    print

retry_boot:
    xorb    %ah, %ah
    int     $0x16
    int     $0x19  



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


//.space          0xF000
error_msg:      .asciz  "Failed to get memory size."
happy_msg:      .asciz  "Yay! You made it!\r\n"
