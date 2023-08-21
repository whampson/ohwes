# =============================================================================
# Copyright (C) 2023 Wes Hampson. All Rights Reserved.
#
# This file is part of the OH-WES Operating System.
# OH-WES is free software; you may redistribute it and/or modify it under the
# terms of the GNU GPLv2. See the LICENSE file in the root of this repository.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# -----------------------------------------------------------------------------
#         File: kernel/interrupt_asm.S
#      Created: December 21, 2020
#       Author: Wes Hampson
#
# Interrupt handling thunk routines. Every interrupt is funnelled into a common
# handler, which saves the process context, then calls the proper handling
# routine depending on the type of interrupt raised.
# =============================================================================

#include <hw/interrupt.h>

IFRAME_EBX              = 0x00
IFRAME_ECX              = 0x04
IFRAME_EDX              = 0x08
IFRAME_ESI              = 0x0C
IFRAME_EDI              = 0x10
IFRAME_EBP              = 0x14
IFRAME_EAX              = 0x18
IFRAME_VEC_NUM          = 0x1C
IFRAME_ERR_CODE         = 0x20
IFRAME_EIP              = 0x24
IFRAME_CS               = 0x28
IFRAME_EFLAGS           = 0x2C
IFRAME_ESP              = 0x30
IFRAME_SS               = 0x34

#
# Common interrupt entry point.
#
# Upon entry, stack contents are as follows:
#   VEC_NUM     <-- stack pointer
#   ERR_CODE
#   EIP             pushed by cpu
#   CS              pushed by cpu
#   EFLAGS          pushed by cpu
#   ESP             pushed by cpu, only present when interrupt switches privilege levels
#   SS              pushed by cpu, only present when interrupt switches privilege levels
#               <-- frame base
#
common_interrupt_handler:
        subl            $0x1C, %esp
        movl            %ebx, IFRAME_EBX(%esp)
        movl            %ecx, IFRAME_ECX(%esp)
        movl            %edx, IFRAME_EDX(%esp)
        movl            %esi, IFRAME_ESI(%esp)
        movl            %edi, IFRAME_EDI(%esp)
        movl            %ebp, IFRAME_EBP(%esp)
        movl            %eax, IFRAME_EAX(%esp)

        movl            %esp, %ecx              # ecx: fastcall arg0 (struct iframe *)

        cmpl            $INT_SYSCALL, IFRAME_VEC_NUM(%ecx)
        je              do_syscall

do_interrupt:
        cmpl            $0, IFRAME_VEC_NUM(%ecx)
        pushl           $interrupt_return       # return to interrupt_return
        js              handle_irq
        jmp             handle_exception

do_syscall:
        pushl           $syscall_return         # return to syscall_return
        jmp             handle_syscall

interrupt_return:
        movl            IFRAME_EAX(%esp), %eax  # eax = return value for syscalls
syscall_return:
        movl            IFRAME_EBP(%esp), %ebp
        movl            IFRAME_EDI(%esp), %edi
        movl            IFRAME_ESI(%esp), %esi
        movl            IFRAME_EDX(%esp), %edx
        movl            IFRAME_ECX(%esp), %ecx
        movl            IFRAME_EBX(%esp), %ebx
        addl            $0x24, %esp
        iret


#
# Below are small thunk functions for linking entries in the IDT to the common
# interrupt handler defined above. There is one thunk for every entry in the
# IDT.
#

#
# Exception thunk routine. Pushes an error code of 0, followed by the exception
# vector number, then jumps to the common interrupt handler.
#
#define THUNK_EXCEPTION(thunk_name, vec_num)                                    \
.globl thunk_name;                                                              \
thunk_name:                                                                     \
        pushl           $0;                                                     \
        pushl           $vec_num;                                               \
        jmp             common_interrupt_handler

#
# Exception thunk routine for an exception that automatically pushes an error
# code. Pushes the exception vector number, then jumps to the common interrupt
# handler.
#
#define THUNK_EXCEPTION_WITH_ERRORCODE(thunk_name, vec_num)                     \
.globl thunk_name;                                                              \
thunk_name:                                                                     \
    pushl   $vec_num;                                                           \
    jmp     common_interrupt_handler

#
# Device IRQ thunk routine. Pushes an error code of 0, followed by the one's
# compliment of the device IRQ number, then jumps to the common interrupt
# handler. The one's compliment of the IRQ number is used to both encode the IRQ
# number and differentiate the IRQ number from an exception vector.
#
#define THUNK_IRQ(thunk_name, irq_num)                                          \
.globl thunk_name;                                                              \
thunk_name:                                                                     \
    pushl   $0;                                                                 \
    pushl   $~irq_num;                                                          \
    jmp     common_interrupt_handler

#
# System call thunk routine. Pushes an error code of 0, followed by the
# system call interrupt vector number, then jumps to the common interrupt
# handler.
#
#define THUNK_SYSCALL(thunk_name)                                               \
.globl thunk_name;                                                              \
thunk_name:                                                                     \
    pushl   $0;                                                                 \
    pushl   $INT_SYSCALL;                                                       \
    jmp     common_interrupt_handler

#
# Below are the thunk routines for all valid interrupt and exception vectors
# in the IDT.
#

# interrupt types:
#     fault - re-excecutes faulting instruction upon return from handler
#      trap - executes instruction following trapping instruction upon return from handler
#     abort - severe error, interrupted program cannot be resumed

# 0-31                          processor exceptions
THUNK_EXCEPTION                 (Exception00h, EXCEPTION_DE)    # fault
THUNK_EXCEPTION                 (Exception01h, EXCEPTION_DB)    # fault/trap
THUNK_EXCEPTION                 (Exception02h, EXCEPTION_NMI)   # interrupt
THUNK_EXCEPTION                 (Exception03h, EXCEPTION_BP)    # trap
THUNK_EXCEPTION                 (Exception04h, EXCEPTION_OF)    # trap
THUNK_EXCEPTION                 (Exception05h, EXCEPTION_BR)    # fault
THUNK_EXCEPTION                 (Exception06h, EXCEPTION_UD)    # fault
THUNK_EXCEPTION                 (Exception07h, EXCEPTION_NM)    # fault
THUNK_EXCEPTION_WITH_ERRORCODE  (Exception08h, EXCEPTION_DF)    # abort, error code is 0
THUNK_EXCEPTION                 (Exception09h, 9)               # fault (not used, was Coprocessor Segment Overrun)
THUNK_EXCEPTION_WITH_ERRORCODE  (Exception0Ah, EXCEPTION_TS)    # fault
THUNK_EXCEPTION_WITH_ERRORCODE  (Exception0Bh, EXCEPTION_NP)    # fault
THUNK_EXCEPTION_WITH_ERRORCODE  (Exception0Ch, EXCEPTION_SS)    # fault
THUNK_EXCEPTION_WITH_ERRORCODE  (Exception0Dh, EXCEPTION_GP)    # fault
THUNK_EXCEPTION_WITH_ERRORCODE  (Exception0Eh, EXCEPTION_PF)    # fault
THUNK_EXCEPTION                 (Exception0Fh, 15)              # (Intel reserved)
THUNK_EXCEPTION                 (Exception10h, EXCEPTION_MF)    # fault
THUNK_EXCEPTION_WITH_ERRORCODE  (Exception11h, EXCEPTION_AC)    # fault, error code is 0
THUNK_EXCEPTION                 (Exception12h, EXCEPTION_MC)    # abort
THUNK_EXCEPTION                 (Exception13h, EXCEPTION_XM)    # fault
THUNK_EXCEPTION                 (Exception14h, EXCEPTION_VE)    # fault
THUNK_EXCEPTION_WITH_ERRORCODE  (Exception15h, EXCEPTION_CP)    # fault
THUNK_EXCEPTION                 (Exception16h, 22)              # (Intel reserved)
THUNK_EXCEPTION                 (Exception17h, 23)              # (Intel reserved)
THUNK_EXCEPTION                 (Exception18h, 24)              # (Intel reserved)
THUNK_EXCEPTION                 (Exception19h, 25)              # (Intel reserved)
THUNK_EXCEPTION                 (Exception1Ah, 26)              # (Intel reserved)
THUNK_EXCEPTION                 (Exception1Bh, 27)              # (Intel reserved)
THUNK_EXCEPTION                 (Exception1Ch, 28)              # (Intel reserved)
THUNK_EXCEPTION                 (Exception1Dh, 29)              # (Intel reserved)
THUNK_EXCEPTION                 (Exception1Eh, 30)              # (Intel reserved)
THUNK_EXCEPTION                 (Exception1Fh, 31)              # (Intel reserved)

# 32-47                         device IRQs
THUNK_IRQ                       (Irq00h,  0)                    # trap
THUNK_IRQ                       (Irq01h, IRQ_KEYBOARD)          # trap
THUNK_IRQ                       (Irq02h, 2)                     # trap
THUNK_IRQ                       (Irq03h, 3)                     # trap
THUNK_IRQ                       (Irq04h, 4)                     # trap
THUNK_IRQ                       (Irq05h, 5)                     # trap
THUNK_IRQ                       (Irq06h, 6)                     # trap
THUNK_IRQ                       (Irq07h, 7)                     # trap
THUNK_IRQ                       (Irq08h, 8)                     # trap
THUNK_IRQ                       (Irq09h, 9)                     # trap
THUNK_IRQ                       (Irq0Ah, 10)                    # trap
THUNK_IRQ                       (Irq0Bh, 11)                    # trap
THUNK_IRQ                       (Irq0Ch, 12)                    # trap
THUNK_IRQ                       (Irq0Dh, 13)                    # trap
THUNK_IRQ                       (Irq0Eh, 14)                    # trap
THUNK_IRQ                       (Irq0Fh, 15)                    # trap

# 48-79                         reserved

# 80                            system call
THUNK_SYSCALL                   (Syscall)                       # trap

# 81-255                        reserved
