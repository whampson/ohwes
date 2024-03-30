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

.text
.code32

#include <interrupt.h>
#include <irq.h>
#include <errno.h>
#include <syscall.h>

.macro LOAD_SEGREG addr, reg
        movw            \addr, %ax
        movw            %ax, \reg
.endm
.macro STORE_SEGREG reg, addr
        movw            \reg, %ax
        movw            %ax, \addr
.endm

.globl kentry
kentry:
        cli
        call            kmain

.globl idle
idle:
        jmp             idle

##
# Common Interrupt Handler
#
# The Common Interrupt Handler pushes the interrupted process' registers onto
# the stack effectively saving the CPU state of the process. Registers are
# pushed in a known order known as an 'iregs' structure. A pointer to that
# structure then is passed onto the interrupt handler.
#
# Registers are pushed onto the stack as follows:
# sp -> ebx        pushed by recv_interrupt, syscall param 0
#       ecx        pushed by recv_interrupt, syscall param 1
#       edx        pushed by recv_interrupt, syscall param 2
#       esi        pushed by recv_interrupt, syscall param 3
#       edi        pushed by recv_interrupt, syscall param 4
#       ebp        pushed by recv_interrupt
#       eax        pushed by recv_interrupt, syscall number / return value
#       ds         pushed by recv_interrupt
#       es         pushed by recv_interrupt
#       fs         pushed by recv_interrupt
#       gs         pushed by recv_interrupt
#       vec_num    pushed by thunk, interrupt vector number, 0x80 for syscall
#       err_code   pushed by cpu or thunk depending on exception
#       eip        pushed by cpu
#       cs         pushed by cpu
#       eflags     pushed by cpu
#       esp        pushed by cpu, only present upon privilege level change
#       ss         pushed by cpu, only present upon privilege level change
# bp ->            (bp loaded by cpu from tss->esp0 upon privilege level change)
#
# After the registers are stored, the type of interrupt is determined using the
# vector number: Generic Interrupt, Device IRQ, or System Call. Then, we jump to
# the corresponding 'trampoline' function, which pushes the return address onto
# the stack then jumps to the appropriate handler function.
 #
# A Generic Interrupt is any interrupt that is not a System Call or Device IRQ.
# These are typically Intel Exceptions, which occupy vectors 0-31. Anything else
# should be considered unexpected and should probably raise an alarm. The iregs
# pointer is passed to the interrupt handler via ECX. As such all interrupt
# handlers with C linkage should be declared with the __fastcall decorator:
#       void __fastcall handler(struct iregs *regs);
#
# A Device IRQ is an interrupt request from a peripheral device connected to the
# CPU via the PIC. The interrupt vector number for a Device IRQ is the ones'
# compliment of the device's IRQ number. As such, the device that raised the
# interrupt can be identified by taking the bitwise NOT of the vector number.
# Device IRQs are expected to be acknowledged in the hardware with an EOI
# (End of Interrupt) message upon completion of the handler, which shall be
# handled by the aggregate handler.
#
# A System Call is a software call into the kernel in order to perform some
# operation that requires privileged execution, such as reading/writing data
# to a device or allocating memory. System Calls always use the vector number
# 0x80. The system function requested is specified via EAX, while the
# parameters to the function are passed via EBX, ECX, EDX, ESI, and EDI (in
# that order with respect to a C-style function signature), for up to five
# 32-bit parameters. The return value is a 32-bit integer passed via EAX.
# A negative return value is an error code. The two's compliment of the error
# code is corresponds to one of the codes defined in errno.h. Standard Library
# system call wrappers are expected to set the 'errno' value to the two's
# complimented error code.
# System Calls are declared on the kernel-side as follows:
#       int sys_getpid(void);
#       int sys_read(int fd, char *buf, unsigned int count);
##
recv_interrupt:
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
        movl            %esp, %ecx      # fastcall
        movl            IREGS_VEC_NUM(%esp), %eax
        test            %eax, %eax
        js              irq_trampoline
        cmpl            $IVT_SYSCALL, %eax
        je              syscall_trampoline
        jmp             interrupt_trampoline

interrupt_trampoline:
        pushl           $end_interrupt
        cmpl            $NUM_EXCEPTIONS, %eax
        ja              _noint
        jmp             *exception_table(, %eax, 4)
_noint:
        jmp             crash          # unexpected interrupt!

irq_trampoline:
        pushl           $end_irq
        jmp             handle_irq      # see irq.c

syscall_trampoline:
        pushl           $end_syscall
        movl            IREGS_EAX(%ecx), %eax
        cmpl            $NUM_SYSCALLS, %eax
        ja              _nosys
        jmp             *syscall_table(, %eax, 4)
_nosys:
        movl            $-ENOSYS, %eax
        ret

syscall_table:
        .long           sys_exit, sys_read, sys_write, sys_open, sys_close
        .long           sys_ioctl

exception_table:
        .long           crash, crash, crash, crash, crash, crash, crash, crash
        .long           crash, crash, crash, crash, crash, crash, crash, crash
        .long           crash, crash, crash, crash, crash, crash, crash, crash
        .long           crash, crash, crash, crash, crash, crash, crash, crash

.globl switch_context
switch_context:
        movl            %ecx, %esp      # fastcall
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
        movl            %eax, IREGS_ERR_CODE(%esp) # reuse error code to
        LOAD_SEGREG     IREGS_DS(%esp), %ds        #  temporarily store eax,
        LOAD_SEGREG     IREGS_ES(%esp), %es        #  used by these macros to
        LOAD_SEGREG     IREGS_FS(%esp), %fs        #  restore segment registers
        LOAD_SEGREG     IREGS_GS(%esp), %gs
        movl            IREGS_ERR_CODE(%esp), %eax
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
        jmp     recv_interrupt

##
# Interrupt thunk w/ error code already pushed by processor
# (for certain Exceptions).
##
#define INTERRUPT_THUNK_ERRCODE(thunk_name, vec_num)                            \
.globl thunk_name;                                                              \
thunk_name:                                                                     \
        pushl   $vec_num;                                                       \
        jmp     recv_interrupt

##
# Device IRQ thunk.
##
#define IRQ_THUNK(thunk_name, vec_num)                                          \
.globl thunk_name;                                                              \
thunk_name:                                                                     \
        pushl   $0;                                                             \
        pushl   $~vec_num;                                                      \
        jmp     recv_interrupt


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
INTERRUPT_THUNK         (_thunk_test, 69)

# 80                    system call
INTERRUPT_THUNK         (_thunk_syscall, IVT_SYSCALL)

# 81-255                reserved
