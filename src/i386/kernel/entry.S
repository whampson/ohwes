# =============================================================================
# Copyright (C) 2020-2025 Wes Hampson. All Rights Reserved.
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
#         File: src/i386/kernel/entry.S
#      Created: December 11, 2020
#       Author: Wes Hampson
# =============================================================================

####
# Kernel Mode Entry Points
####

.text
.code32

#include <errno.h>
#include <i386/interrupt.h>
#include <i386/syscall.h>
#include <i386/x86.h>
#include <kernel/irq.h>
#include <kernel/ohwes.h>

##
# recv_interrupt:
# Common Interrupt Handler
#
# The Common Interrupt Handler serves as the main entry point for all interrupts
# (traps) received by the kernel. Its purpose is to save interrupted process'
# state, determine the type of interrupt that occurred, set up the return stack,
# then pass control on to the appropriate interrupt service routine. The process
# state is stored on the stack in an 'iregs' structure, to be restored upon
# completion of the routine.
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
# vector number: Exception, Device IRQ, or System Call. We then jump to the
# corresponding 'trampoline' function, which pushes the appropriate return
# address onto the stack then jumps to the corresponding handler function. A
# pointer to the iregs structure is passed to the interrupt handler via ECX. As
# such, handlers with C linkage callable directly from the common interrupt
# handler should be declared with the __fastcall decorator:
#       void __fastcall handler(struct iregs *regs);
#
# An Exception is any programmatic interrupt that is not a system call. These
# are typically one of the Intel Exceptions, which occupy vectors 0-31, and are
# raised due to a fault in the executing program, such a divide-by-zero error or
# attempt to execute a privileged instruction with insufficient privilege. Any
# other kind of exception is considered unexpected and should probably raise
# alarm! Exceptions do not return a meaningful value of any sort. In fact, they
# might not return at all! Exceptions may affect program behavior in order to
# prevent a system crash.
#
# A Device IRQ is an interrupt request from a peripheral device. Devices are
# connected to the CPU via the PIC. The device IRQ number is encoded as the
# ones' compliment of the interrupt vector number. In other words, the device
# that raised the interrupt can be identified by taking the bitwise NOT of the
# vector number. Device IRQs are expected by the PIC to be acknowledged with an
# End of Interrupt (EOI) message upon completion of the handler. This is to be
# taken care of by the interrupt handler. Like exceptions, device IRQs do not
# return any meaningful value to the user, but unlike exceptions they are always
# expected to return.
#
# A System Call is a software call into the kernel in order to perform some
# privileged operation, such as reading/writing data to a device or allocating
# memory. System calls are invoked using the 'int' instruction and always use
# the interrupt vector number 0x80. The requested kernel function is specified
# as a 32-bit number in EAX, while the parameters to the function are passed via
# EBX, ECX, EDX, ESI, and EDI registers, in that order with respect to a C-style
# function signature, for a total of up to five 32-bit parameters. The return
# value is a 32-bit integer passed back to the calling program via EAX. These
# parameters are already on the stack thanks to our common interrupt handler so
# we simply need to call the requested function. To ensure that system calls
# with C linkage only look at the stack for parameters, we declare the kernel-
# side of a system call using the __syscall decorator:
#       int __syscall sys_read(int fd, char *buf, size_t count);
#
# A negative return value from a system call indicates an error. The two's
# compliment of the value returned is an error code that corresponds to one of
# the codes defined in errno.h. System call wappers used by the C Standard
# Library are expected to set the 'errno' value to this error code and return
# -1 back to the user.
##
recv_interrupt:
        cli
        subl            $SIZEOF_IREGS_CTX_REGS, %esp
        movl            %ebx, IREGS_EBX(%esp)
        movl            %ecx, IREGS_ECX(%esp)
        movl            %edx, IREGS_EDX(%esp)
        movl            %esi, IREGS_ESI(%esp)
        movl            %edi, IREGS_EDI(%esp)
        movl            %ebp, IREGS_EBP(%esp)
        movl            %eax, IREGS_EAX(%esp)
        STORE_SEGMENT   %ds, IREGS_DS(%esp)
        STORE_SEGMENT   %es, IREGS_ES(%esp)
        STORE_SEGMENT   %fs, IREGS_FS(%esp)
        STORE_SEGMENT   %gs, IREGS_GS(%esp)
        movl            %esp, %ecx                      # ecx = fastcall param
        movl            IREGS_VEC_NUM(%esp), %eax
        test            %eax, %eax
        js              irq_trampoline
        cmpl            $VEC_SYSCALL, %eax
        je              syscall_trampoline
        jmp             exception_trampoline

exception_trampoline:
        pushl           $return_from_exception
        cmpl            $NR_EXCEPTIONS, %eax
        ja              _noint
        jmp             *exception_table(, %eax, 4)  # handle exception
_noint:
        jmp             _crash

irq_trampoline:
        pushl           $return_from_irq
        jmp             handle_irq      # common IRQ handler

syscall_trampoline:
        movl            IREGS_EAX(%ecx), %eax
        cmpl            $NR_SYSCALLS, %eax
        ja              _nosys
        jmp             sys_enter
_nosys:
        movl            $-ENOSYS, %eax          # ENOSYS = no such syscall exists
        jmp             return_from_syscall

sys_enter:
        pushl           $sys_leave
        pushl           %eax
        call            push_kernel_stack       # make sure any nested interrupt uses a different kernel stack!
        popl            %eax
        sti
        jmp             *syscall_table(, %eax, 4)

sys_leave:
        cli
        pushl           %eax
        call            pop_kernel_stack
        popl            %eax
        jmp             return_from_syscall

# ----------------------------------------------------------------------------
# interrupt return / task switch

# context switch
.globl switch_context
switch_context:
        cli
        movl            %ecx, %esp      # fastcall param
                                        # fallthrough...
return_from_exception:
return_from_irq:
        movl            IREGS_EAX(%esp), %eax
return_from_syscall:
        movl            IREGS_EBP(%esp), %ebp
        movl            IREGS_EDI(%esp), %edi
        movl            IREGS_ESI(%esp), %esi
        movl            IREGS_EDX(%esp), %edx
        movl            IREGS_ECX(%esp), %ecx
        movl            IREGS_EBX(%esp), %ebx
        movl            %eax, IREGS_ERR_CODE(%esp) # reuse error code slot to
        LOAD_SEGMENT    IREGS_DS(%esp), %ds        #  temporarily store eax,
        LOAD_SEGMENT    IREGS_ES(%esp), %es        #  used by these macros to
        LOAD_SEGMENT    IREGS_FS(%esp), %fs        #  restore segment registers
        LOAD_SEGMENT    IREGS_GS(%esp), %gs
        movl            IREGS_ERR_CODE(%esp), %eax # restore eax
        addl            $IREGS_EIP, %esp
        iret

# ----------------------------------------------------------------------------
# tables

exception_table:
        .long   _crash, _dbgbrk, _crash, _dbgbrk
        .long   _crash, _crash, _crash, _crash
        .long   _crash, _crash, _crash, _crash
        .long   _crash, _crash, _crash, _crash
        .long   _crash, _crash, _crash, _crash
        .long   _crash, _crash, _crash, _crash
        .long   _crash, _crash, _crash, _crash
        .long   _crash, _crash, _crash, _crash

syscall_table:
        #include <i386/syscall_table.h>
_syscall_table_end:

NR_SYSCALLS = (_syscall_table_end - syscall_table) >> 2

# ----------------------------------------------------------------------------
# Below are thunk routines for all valid entries in the IDT.
#
# A thunk routine is a small non-returning function that is called by the CPU
# via the IDT when an interrupt is raised. These thunk routines push the
# interrupt vector or device IRQ number onto the stack, along with a 32-bit pad
# value of zero for interrupts that do not automatically push an error code,
# before jumping to the common interrupt handler.

##
# Intel exception thunk.
# Pushes a zero in lieu of an error code.
##
#define EXCEPTION_THUNK(thunk_name, vec_num)                                    \
.globl thunk_name;                                                              \
thunk_name:                                                                     \
        pushl   $0;                                                             \
        pushl   $vec_num;                                                       \
        jmp     recv_interrupt

##
# Intel exception thunk with error code already pushed by CPU.
##
#define EXCEPTION_THUNK_ERRCODE(thunk_name, vec_num)                            \
.globl thunk_name;                                                              \
thunk_name:                                                                     \
        pushl   $vec_num;                                                       \
        jmp     recv_interrupt

##
# Device IRQ thunk.
# Pushes the ones' compliment of the IRQ line number.
##
#define IRQ_THUNK(thunk_name, irq_num)                                          \
.globl thunk_name;                                                              \
thunk_name:                                                                     \
        pushl   $0;                                                             \
        pushl   $~irq_num;                                                      \
        jmp     recv_interrupt


# exception types:
#     fault - re-excecutes faulting instruction upon return from handler
#      trap - executes instruction following trapping instruction upon return from handler
#     abort - severe error, interrupted program cannot be resumed

# 0-31                  processor exceptions               classification
EXCEPTION_THUNK         (_except_00h, EXCEPTION_DE)    # fault
EXCEPTION_THUNK         (_except_01h, EXCEPTION_DB)    # fault or trap
EXCEPTION_THUNK         (_except_02h, EXCEPTION_NMI)   # n/a
EXCEPTION_THUNK         (_except_03h, EXCEPTION_BP)    # trap
EXCEPTION_THUNK         (_except_04h, EXCEPTION_OF)    # trap
EXCEPTION_THUNK         (_except_05h, EXCEPTION_BR)    # fault
EXCEPTION_THUNK         (_except_06h, EXCEPTION_UD)    # fault
EXCEPTION_THUNK         (_except_07h, EXCEPTION_NM)    # fault
EXCEPTION_THUNK_ERRCODE (_except_08h, EXCEPTION_DF)    # abort; error code is 0
EXCEPTION_THUNK         (_except_09h, 9)               # abort; not used, was Coprocessor Segment Overrun
EXCEPTION_THUNK_ERRCODE (_except_0Ah, EXCEPTION_TS)    # fault
EXCEPTION_THUNK_ERRCODE (_except_0Bh, EXCEPTION_NP)    # fault
EXCEPTION_THUNK_ERRCODE (_except_0Ch, EXCEPTION_SS)    # fault
EXCEPTION_THUNK_ERRCODE (_except_0Dh, EXCEPTION_GP)    # fault
EXCEPTION_THUNK_ERRCODE (_except_0Eh, EXCEPTION_PF)    # fault
EXCEPTION_THUNK         (_except_0Fh, 15)              # (Intel reserved)
EXCEPTION_THUNK         (_except_10h, EXCEPTION_MF)    # fault
EXCEPTION_THUNK_ERRCODE (_except_11h, EXCEPTION_AC)    # fault
EXCEPTION_THUNK         (_except_12h, EXCEPTION_MC)    # abort
EXCEPTION_THUNK         (_except_13h, EXCEPTION_XM)    # fault
EXCEPTION_THUNK         (_except_14h, EXCEPTION_VE)    # fault
EXCEPTION_THUNK_ERRCODE (_except_15h, EXCEPTION_CP)    # fault
EXCEPTION_THUNK         (_except_16h, 22)              # (Intel reserved)
EXCEPTION_THUNK         (_except_17h, 23)              # (Intel reserved)
EXCEPTION_THUNK         (_except_18h, 24)              # (Intel reserved)
EXCEPTION_THUNK         (_except_19h, 25)              # (Intel reserved)
EXCEPTION_THUNK         (_except_1Ah, 26)              # (Intel reserved)
EXCEPTION_THUNK         (_except_1Bh, 27)              # (Intel reserved)
EXCEPTION_THUNK         (_except_1Ch, 28)              # (Intel reserved)
EXCEPTION_THUNK         (_except_1Dh, 29)              # (Intel reserved)
EXCEPTION_THUNK         (_except_1Eh, 30)              # (Intel reserved)
EXCEPTION_THUNK         (_except_1Fh, 31)              # (Intel reserved)

# 32-47                 device IRQs
IRQ_THUNK               (_irq_00h, IRQ_TIMER)
IRQ_THUNK               (_irq_01h, IRQ_KEYBOARD)
IRQ_THUNK               (_irq_02h, IRQ_SLAVE)
IRQ_THUNK               (_irq_03h, IRQ_COM2)
IRQ_THUNK               (_irq_04h, IRQ_COM1)
IRQ_THUNK               (_irq_05h, IRQ_LPT2)
IRQ_THUNK               (_irq_06h, IRQ_FLOPPY)
IRQ_THUNK               (_irq_07h, IRQ_LPT1)
IRQ_THUNK               (_irq_08h, IRQ_RTC)
IRQ_THUNK               (_irq_09h, IRQ_ACPI)
IRQ_THUNK               (_irq_0Ah, IRQ_MISC1)
IRQ_THUNK               (_irq_0Bh, IRQ_MISC2)
IRQ_THUNK               (_irq_0Ch, IRQ_MOUSE)
IRQ_THUNK               (_irq_0Dh, IRQ_COPOCESSOR)
IRQ_THUNK               (_irq_0Eh, IRQ_ATA1)
IRQ_THUNK               (_irq_0Fh, IRQ_ATA2)

# 48-79                 reserved

# 80                    system call
EXCEPTION_THUNK         (syscall_thunk, VEC_SYSCALL)

# 81-255                reserved

# -----------------------------------------------------------------------------

.globl exception_thunks
exception_thunks:
        .long           _except_00h, _except_01h, _except_02h, _except_03h
        .long           _except_04h, _except_05h, _except_06h, _except_07h
        .long           _except_08h, _except_09h, _except_0Ah, _except_0Bh
        .long           _except_0Ch, _except_0Dh, _except_0Eh, _except_0Fh
        .long           _except_10h, _except_11h, _except_12h, _except_13h
        .long           _except_14h, _except_15h, _except_16h, _except_17h
        .long           _except_18h, _except_19h, _except_1Ah, _except_19h
        .long           _except_1Ch, _except_1Ch, _except_1Eh, _except_1Fh

.globl irq_thunks
irq_thunks:
        .long           _irq_00h, _irq_01h, _irq_02h, _irq_03h
        .long           _irq_04h, _irq_05h, _irq_06h, _irq_07h
        .long           _irq_08h, _irq_09h, _irq_0Ah, _irq_0Bh
        .long           _irq_0Ch, _irq_0Dh, _irq_0Eh, _irq_0Fh
