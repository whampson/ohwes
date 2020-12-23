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
#    File: kernel/interrupt_thunk.S                                            #
# Created: December 21, 2020                                                   #
#  Author: Wes Hampson                                                         #
#                                                                              #
# Interrupt handling thunk routines. Every interrupt is funnelled into a       #
# common handler, which saves the process context, then calls the proper       #
# handling routine depending on the type of interrupt raised.                  #
#==============================================================================#

#include <ohwes/interrupt.h>
#include <ohwes/except.h>
#include <ohwes/irq.h>

IFRAME_EBX          = 0x00
IFRAME_ECX          = 0x04
IFRAME_EDX          = 0x08
IFRAME_ESI          = 0x0C
IFRAME_EDI          = 0x10
IFRAME_EBP          = 0x14
IFRAME_EAX          = 0x18
IFRAME_VEC_NUM      = 0x1C
IFRAME_ERR_CODE     = 0x20
IFRAME_EIP          = 0x24
IFRAME_CS           = 0x28
IFRAME_EFLAGS       = 0x2C
IFRAME_ESP          = 0x30
IFRAME_SS           = 0x34

common_interrupt:
    subl    $0x1C, %esp
    movl    %ebx, IFRAME_EBX(%esp)
    movl    %ecx, IFRAME_ECX(%esp)
    movl    %edx, IFRAME_EDX(%esp)
    movl    %esi, IFRAME_ESI(%esp)
    movl    %edi, IFRAME_EDI(%esp)
    movl    %ebp, IFRAME_EBP(%esp)
    movl    %eax, IFRAME_EAX(%esp)

    movl    %esp, %ecx              # ecx: fastcall arg0 (struct iframe *)

    cmpl    $INT_SYSCALL, IFRAME_VEC_NUM(%esp)
    je      do_syscall
    cmpl    $0, IFRAME_VEC_NUM(%esp)
    js      do_irq
    jmp     do_except

do_syscall:
    pushl   $syscall_return
    jmp     system_call

do_irq:
    call    handle_irq
    jmp     interrupt_return

do_except:
    call    handle_except
    jmp     interrupt_return

interrupt_return:
    movl    IFRAME_EAX(%esp), %eax
syscall_return:
    movl    IFRAME_EBP(%esp), %ebp
    movl    IFRAME_EDI(%esp), %edi
    movl    IFRAME_ESI(%esp), %esi
    movl    IFRAME_EDX(%esp), %edx
    movl    IFRAME_ECX(%esp), %ecx
    movl    IFRAME_EBX(%esp), %ebx
    addl    $0x24, %esp
    iret


/* Below are small thunk functions for linking entries in the IDT to the common
   interrupt handler defined above. There exists one of these thunk functions
   for every active entry in the IDT. */

#define EXCEPT_THUNK(thunk_name, vec_num)                   \
.globl thunk_name;                                          \
thunk_name:                                                 \
    pushl   $0;                                             \
    pushl   $vec_num;                                       \
    jmp     common_interrupt

#define EXCEPT_THUNK_WITH_ERRORCODE(thunk_name, vec_num)    \
.globl thunk_name;                                          \
thunk_name:                                                 \
    pushl   $vec_num;                                       \
    jmp     common_interrupt

#define IRQ_THUNK(thunk_name, irq_num)                      \
.globl thunk_name;                                          \
thunk_name:                                                 \
    pushl   $0;                                             \
    pushl   $~irq_num;                                      \
    jmp     common_interrupt

#define SYSCALL_THUNK(thunk_name)                           \
.globl thunk_name;                                          \
thunk_name:                                                 \
    pushl   $0;                                             \
    pushl   $INT_SYSCALL;                                   \
    jmp     common_interrupt

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
