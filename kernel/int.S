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
#    File: kernel/intr.S                                                       #
# Created: December 21,2020                                                    #
#  Author: Wes Hampson                                                         #
#                                                                              #
# Interrupt handling routines. Every interrupt is funnelled into a common      #
# handler, which saves the process context, then calls the proper handling     #
# routine depending on the type of interrupt raised.                           #
#==============================================================================#

#include <ohwes/irq.h>
#include <ohwes/exception.h>
#include <ohwes/interrupt.h>

common_handler:
    iret

/* Below are small thunk functions for linking entries in the IDT to the common
   interrupt handler defined above. There exists one of these thunk functions
   for every active entry in the IDT. */

#define EXCEPT_THUNK(thunk_name, vec_num)                   \
.globl thunk_name;                                          \
thunk_name:                                                 \
    pushl   $-1;                                            \
    pushl   $vec_num;                                       \
    jmp     common_handler

#define EXCEPT_THUNK_WITH_ERRORCODE(thunk_name, vec_num)    \
.globl thunk_name;                                          \
thunk_name:                                                 \
    pushl   $vec_num;                                       \
    jmp     common_handler

#define IRQ_THUNK(thunk_name, irq_num)                      \
.globl thunk_name;                                          \
thunk_name:                                                 \
    pushl   $-1;                                            \
    pushl   $~irq_num;                                      \
    jmp     common_handler

#define SYSCALL_THUNK(thunk_name)                           \
.globl thunk_name;                                          \
thunk_name:                                                 \
    pushl   $-1;                                            \
    pushl   $SYSCALL_VECTOR;                                \
    jmp     common_handler

EXCEPT_THUNK                (thunk_except_00, EXCEPT_DE)
EXCEPT_THUNK                (thunk_except_01, EXCEPT_DB)
EXCEPT_THUNK                (thunk_except_02, EXCEPT_NMI)
EXCEPT_THUNK                (thunk_except_03, EXCEPT_BP)
EXCEPT_THUNK                (thunk_except_04, EXCEPT_OF)
EXCEPT_THUNK                (thunk_except_05, EXCEPT_BR)
EXCEPT_THUNK                (thunk_except_06, EXCEPT_UD)
EXCEPT_THUNK                (thunk_except_07, EXCEPT_NM)
EXCEPT_THUNK_WITH_ERRORCODE (thunk_except_08, EXCEPT_DF)
EXCEPT_THUNK                (thunk_except_09, 9)
EXCEPT_THUNK_WITH_ERRORCODE (thunk_except_10, EXCEPT_TS)
EXCEPT_THUNK_WITH_ERRORCODE (thunk_except_11, EXCEPT_NP)
EXCEPT_THUNK_WITH_ERRORCODE (thunk_except_12, EXCEPT_SS)
EXCEPT_THUNK_WITH_ERRORCODE (thunk_except_13, EXCEPT_GP)
EXCEPT_THUNK_WITH_ERRORCODE (thunk_except_14, EXCEPT_PF)
EXCEPT_THUNK                (thunk_except_15, 15)
EXCEPT_THUNK                (thunk_except_16, EXCEPT_MF)
EXCEPT_THUNK_WITH_ERRORCODE (thunk_except_17, EXCEPT_AC)
EXCEPT_THUNK                (thunk_except_18, EXCEPT_MC)
EXCEPT_THUNK                (thunk_except_19, EXCEPT_XM)
EXCEPT_THUNK                (thunk_except_20, EXCEPT_VE)
EXCEPT_THUNK_WITH_ERRORCODE (thunk_except_21, EXCEPT_CP)
EXCEPT_THUNK                (thunk_except_22, 22)
EXCEPT_THUNK                (thunk_except_23, 23)
EXCEPT_THUNK                (thunk_except_24, 24)
EXCEPT_THUNK                (thunk_except_25, 25)
EXCEPT_THUNK                (thunk_except_26, 26)
EXCEPT_THUNK                (thunk_except_27, 27)
EXCEPT_THUNK                (thunk_except_28, 28)
EXCEPT_THUNK                (thunk_except_29, 29)
EXCEPT_THUNK                (thunk_except_30, 30)
EXCEPT_THUNK                (thunk_except_31, 31)
IRQ_THUNK                   (thunk_irq_00,  0)
IRQ_THUNK                   (thunk_irq_01, IRQ_KEYBOARD)
IRQ_THUNK                   (thunk_irq_02, 2)
IRQ_THUNK                   (thunk_irq_03, 3)
IRQ_THUNK                   (thunk_irq_04, 4)
IRQ_THUNK                   (thunk_irq_05, 5)
IRQ_THUNK                   (thunk_irq_06, 6)
IRQ_THUNK                   (thunk_irq_07, 7)
IRQ_THUNK                   (thunk_irq_08, 8)
IRQ_THUNK                   (thunk_irq_09, 9)
IRQ_THUNK                   (thunk_irq_10, 10)
IRQ_THUNK                   (thunk_irq_11, 11)
IRQ_THUNK                   (thunk_irq_12, 12)
IRQ_THUNK                   (thunk_irq_13, 13)
IRQ_THUNK                   (thunk_irq_14, 14)
IRQ_THUNK                   (thunk_irq_15, 15)
SYSCALL_THUNK               (thunk_syscall)
