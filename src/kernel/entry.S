# =============================================================================
# Copyright (C) 2020-2024 Wes Hampson. All Rights Reserved.
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
#         File: kernel/entry.S
#      Created: December 11, 2020
#       Author: Wes Hampson
# =============================================================================

#include <interrupt.h>
#include <irq.h>

.text
.code32

# ----------------------------------------------------------------------------

.globl kentry
kentry:
        cli
        call            kmain

.globl halt
halt:
        hlt
        jmp             halt


.globl idle
idle:
        jmp             idle

# ----------------------------------------------------------------------------

.macro LOAD_SEGREG addr, reg
        movw            \addr, %ax
        movw            %ax, \reg
.endm

.macro STORE_SEGREG reg, addr
        movw            \reg, %ax
        movw            %ax, \addr
.endm

##
# Common interrupt entry point.
#
# Upon entry, stack contents are as follows:
# sp --> vec_num         pushed by thunk
#        err_code        pushed by cpu or thunk
#        eip             pushed by cpu
#        cs              pushed by cpu
#        eflags          pushed by cpu
#        esp             pushed by cpu, only present when interrupt switches privilege levels
#        ss              pushed by cpu, only present when interrupt switches privilege levels
# bp -->                 (bp loaded by cpu from tss->esp0)
##
interrupt:
        cli
        subl            $SIZEOF_IREGS_CTX, %esp
        movl            %ebx, IREGS_EBX(%esp)
        movl            %ecx, IREGS_ECX(%esp)
        movl            %edx, IREGS_EDX(%esp)
        movl            %esi, IREGS_ESI(%esp)
        movl            %edi, IREGS_EDI(%esp)
        movl            %ebp, IREGS_EBP(%esp)
        movl            %eax, IREGS_EAX(%esp)
        STORE_SEGREG    %ds, IREGS_DS(%esp)
        STORE_SEGREG    %es, IREGS_ES(%esp)
        STORE_SEGREG    %fs, IREGS_FS(%esp)
        STORE_SEGREG    %gs, IREGS_GS(%esp)
        movl            %esp, %ecx              # fastcall param (struct iregs *)
        movl            IREGS_VEC_NUM(%esp), %eax
        test            %eax, %eax
        js              irq_trampoline
        cmpl            $IVT_SYSCALL, %eax
        je              syscall_trampoline
        jmp             interrupt_trampoline

        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        # noncontiguous execution
        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interrupt_trampoline:
        pushl           $end_interrupt
        jmp             recv_interrupt

irq_trampoline:
        pushl           $end_irq
        jmp             recv_irq

syscall_trampoline:
        pushl           $end_syscall
        jmp             recv_syscall

temp_eax:
        .long           0

.globl switch_context   # fastcall void switch_context(struct iregs *)
switch_context:
        movl            %ecx, %esp
end_interrupt:
end_irq:
        movl            IREGS_EAX(%esp), %eax
end_syscall:
        movl            IREGS_EBP(%esp), %ebp
        movl            IREGS_EDI(%esp), %edi
        movl            IREGS_ESI(%esp), %esi
        movl            IREGS_EDX(%esp), %edx
        movl            IREGS_ECX(%esp), %ecx
        movl            IREGS_EBX(%esp), %ebx
        movl            %eax, temp_eax
        LOAD_SEGREG     IREGS_DS(%esp), %ds
        LOAD_SEGREG     IREGS_ES(%esp), %es
        LOAD_SEGREG     IREGS_FS(%esp), %fs
        LOAD_SEGREG     IREGS_GS(%esp), %gs
        movl            temp_eax, %eax
        addl            $IREGS_EIP, %esp
        iret

# ----------------------------------------------------------------------------
# Below are the thunk routines for all valid entries in the IDT.
#
# A thunk routine (or "stub") is a small function pointed to by an interrupt
# vector which pushes the interrupt or device IRQ number onto the stack,
# along with an optional error code, then calls the common interrupt handler.
# The common interrupt handler serves as a "funnel" for all interrupts to flow
# into for common tasks like storing registers. Interrupt requests are then
# routed to their proper handlers (interrupt service routines). Device IRQs are
# deleated from interrupt or exception numbers by one's compliment. That is,
# one may retireve the actual device IRQ number by taking the one's compliment
# of the parameter passed by the thunk routine as the interrupt number. As such,
# one may identify that an interrupt number in fact represents an IRQ number by
# noting that the sign bit is set.
# ----------------------------------------------------------------------------

##
# Generic interrupt thunk.
##
#define INTERRUPT_THUNK(thunk_name, vec_num)                                    \
.globl thunk_name;                                                              \
thunk_name:                                                                     \
        pushl   $0;                                                             \
        pushl   $vec_num;                                                       \
        jmp     interrupt

##
# Interrupt thunk w/ error code already pushed by processor
# (for certain Exceptions).
##
#define INTERRUPT_THUNK_ERRCODE(thunk_name, vec_num)                            \
.globl thunk_name;                                                              \
thunk_name:                                                                     \
        pushl   $vec_num;                                                       \
        jmp     interrupt

##
# Device IRQ thunk.
##
#define IRQ_THUNK(thunk_name, vec_num)                                          \
.globl thunk_name;                                                              \
thunk_name:                                                                     \
        pushl   $0;                                                             \
        pushl   $~vec_num;                                                      \
        jmp     interrupt


# exception types:
#     fault - re-excecutes faulting instruction upon return from handler
#      trap - executes instruction following trapping instruction upon return from handler
#     abort - severe error, interrupted program cannot be resumed

# 0-31                  processor exceptions               classification
INTERRUPT_THUNK         (_thunk_except00h, EXCEPTION_DE)    # fault
INTERRUPT_THUNK         (_thunk_except01h, EXCEPTION_DB)    # fault/trap
INTERRUPT_THUNK         (_thunk_except02h, EXCEPTION_NMI)   # n/a
INTERRUPT_THUNK         (_thunk_except03h, EXCEPTION_BP)    # trap
INTERRUPT_THUNK         (_thunk_except04h, EXCEPTION_OF)    # trap
INTERRUPT_THUNK         (_thunk_except05h, EXCEPTION_BR)    # fault
INTERRUPT_THUNK         (_thunk_except06h, EXCEPTION_UD)    # fault
INTERRUPT_THUNK         (_thunk_except07h, EXCEPTION_NM)    # fault
INTERRUPT_THUNK_ERRCODE (_thunk_except08h, EXCEPTION_DF)    # abort; error code is 0
INTERRUPT_THUNK         (_thunk_except09h, 9)               # fault (not used, was Coprocessor Segment Overrun)
INTERRUPT_THUNK_ERRCODE (_thunk_except0Ah, EXCEPTION_TS)    # fault
INTERRUPT_THUNK_ERRCODE (_thunk_except0Bh, EXCEPTION_NP)    # fault
INTERRUPT_THUNK_ERRCODE (_thunk_except0Ch, EXCEPTION_SS)    # fault
INTERRUPT_THUNK_ERRCODE (_thunk_except0Dh, EXCEPTION_GP)    # fault
INTERRUPT_THUNK_ERRCODE (_thunk_except0Eh, EXCEPTION_PF)    # fault
INTERRUPT_THUNK         (_thunk_except0Fh, 15)              # (Intel reserved)
INTERRUPT_THUNK         (_thunk_except10h, EXCEPTION_MF)    # fault
INTERRUPT_THUNK_ERRCODE (_thunk_except11h, EXCEPTION_AC)    # fault, error code is 0
INTERRUPT_THUNK         (_thunk_except12h, EXCEPTION_MC)    # abort
INTERRUPT_THUNK         (_thunk_except13h, EXCEPTION_XM)    # fault
INTERRUPT_THUNK         (_thunk_except14h, EXCEPTION_VE)    # fault
INTERRUPT_THUNK_ERRCODE (_thunk_except15h, EXCEPTION_CP)    # fault
INTERRUPT_THUNK         (_thunk_except16h, 22)              # (Intel reserved)
INTERRUPT_THUNK         (_thunk_except17h, 23)              # (Intel reserved)
INTERRUPT_THUNK         (_thunk_except18h, 24)              # (Intel reserved)
INTERRUPT_THUNK         (_thunk_except19h, 25)              # (Intel reserved)
INTERRUPT_THUNK         (_thunk_except1Ah, 26)              # (Intel reserved)
INTERRUPT_THUNK         (_thunk_except1Bh, 27)              # (Intel reserved)
INTERRUPT_THUNK         (_thunk_except1Ch, 28)              # (Intel reserved)
INTERRUPT_THUNK         (_thunk_except1Dh, 29)              # (Intel reserved)
INTERRUPT_THUNK         (_thunk_except1Eh, 30)              # (Intel reserved)
INTERRUPT_THUNK         (_thunk_except1Fh, 31)              # (Intel reserved)

# 32-47                 device IRQs
IRQ_THUNK               (_thunk_irq00h, IRQ_TIMER)
IRQ_THUNK               (_thunk_irq01h, IRQ_KEYBOARD)
IRQ_THUNK               (_thunk_irq02h, IRQ_SLAVE)
IRQ_THUNK               (_thunk_irq03h, IRQ_COM2)
IRQ_THUNK               (_thunk_irq04h, IRQ_COM1)
IRQ_THUNK               (_thunk_irq05h, IRQ_LPT2)
IRQ_THUNK               (_thunk_irq06h, IRQ_FLOPPY)
IRQ_THUNK               (_thunk_irq07h, IRQ_LPT1)
IRQ_THUNK               (_thunk_irq08h, IRQ_RTC)
IRQ_THUNK               (_thunk_irq09h, IRQ_ACPI)
IRQ_THUNK               (_thunk_irq0Ah, IRQ_MISC1)
IRQ_THUNK               (_thunk_irq0Bh, IRQ_MISC2)
IRQ_THUNK               (_thunk_irq0Ch, IRQ_MOUSE)
IRQ_THUNK               (_thunk_irq0Dh, IRQ_COPOCESSOR)
IRQ_THUNK               (_thunk_irq0Eh, IRQ_ATA1)
IRQ_THUNK               (_thunk_irq0Fh, IRQ_ATA2)

# 48-79                 reserved

# 80                    system call
INTERRUPT_THUNK         (_thunk_syscall, IVT_SYSCALL)

# 81-255                reserved
