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
#         File: i386/kernel/entry.S
#      Created: December 11, 2020
#       Author: Wes Hampson
#
# Runtime entry points for the kernel.
# =============================================================================

.text
.code32

#include <errno.h>
#include <i386/interrupt.h>
#include <i386/syscall.h>
#include <i386/x86.h>
#include <kernel/irq.h>
#include <kernel/ohwes.h>

##
# -----------------------------------------------------------------------------
# recv_interrupt
# -----------------------------------------------------------------------------
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
# sp -> ebx        pushed by recv_interrupt; syscall param 0
#       ecx        pushed by recv_interrupt; syscall param 1
#       edx        pushed by recv_interrupt; syscall param 2
#       esi        pushed by recv_interrupt; syscall param 3
#       edi        pushed by recv_interrupt; syscall param 4
#       ebp        pushed by recv_interrupt
#       eax        pushed by recv_interrupt; syscall number / return value
#       ds         pushed by recv_interrupt
#       es         pushed by recv_interrupt
#       fs         pushed by recv_interrupt
#       gs         pushed by recv_interrupt
#       vec_num    pushed by thunk; interrupt vector number, 0x80 for syscall
#       err_code   pushed by cpu or thunk depending on exception
#       eip        pushed by cpu
#       cs         pushed by cpu
#       eflags     pushed by cpu
#       esp        pushed by cpu; only present upon privilege level change
#       ss         pushed by cpu; only present upon privilege level change
# bp ->            (bp loaded by cpu from tss->esp0 upon privilege level change)
#
# After the registers are stored, the type of interrupt is determined using the
# vector number: Exception, Device IRQ, or System Call. We then jump to the
# corresponding 'trampoline' function, which pushes the appropriate return
# address for the type of interrupt onto the stack then jumps to the
# corresponding handler function. A pointer to the iregs structure is passed to
# the interrupt handler via ECX. As such, handlers with C linkage callable
# directly from the common interrupt handler should be declared with the
# __fastcall decorator:
#
#       void __fastcall exception_or_irq_handler(struct iregs *);
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
# connected to the CPU via the Programmable Interrupt Controller (PIC). The
# device IRQ number is encoded as the ones' compliment of the interrupt vector
# number. This allows us to both determine that an interrupt originated
# from a device and the device that raised the interrupt simply by taking the
# bitwise NOT of the vector number! Device IRQs are expected by the PIC to be
# acknowledged with an End of Interrupt (EOI) message upon completion of the
# handler. Like exceptions, device IRQs do not return any meaningful value to
# the user, but unlike exceptions they are always expected to return.
#
# A System Call is a software call into the kernel in order to perform some
# privileged operation, such as reading/writing data to a device or allocating
# memory. System calls are invoked using the 'int' instruction and always use
# the interrupt vector number 0x80. The requested kernel function is specified
# as a 32-bit number in EAX, while the parameters to the function are passed via
# EBX, ECX, EDX, ESI, and EDI registers (in that order with respect to a C-style
# function signature) for a total of up to five 32-bit parameters. Our common
# interrupt handler places these parameters on the stack while it's saving
# program state, allowing us to simply call the requested function. To ensure
# that system calls with C linkage retrieve their parameters from the stack
# only, we declare the kernel side of a system call using the __syscall
# decorator:
#
#       int __syscall system_func(int param0, char *param1, size_t param2);
#
# The return value of a system call is a 32-bit integer passed back to the
# calling program via EAX. A negative return value from a system call indicates
# an error. The two's compliment of the value returned shall be an error code
# that corresponds to one of the codes defined in errno.h. System call wappers
# used by the C Standard Library are expected to set the 'errno' value to this
# code and return -1 back to the user.
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
        movl            %esp, %ebp
        movl            %esp, %ecx              # ecx = fastcall param for iregs
        movl            IREGS_VEC_NUM(%esp), %eax
        test            %eax, %eax
        js              irq_trampoline
        cmpl            $SYSCALL_VECTOR, %eax
        je              syscall_trampoline
        jmp             exception_trampoline

irq_trampoline:
        pushl           $return_from_irq
        jmp             handle_irq

exception_trampoline:
        pushl           $return_from_exception
        jmp             handle_exception

syscall_trampoline:
        movl            IREGS_EAX(%ecx), %eax
        cmpl            $NR_SYSCALLS, %eax
        ja              _nosys
        jmp             sys_enter
_nosys:
        movl            $-ENOSYS, %eax          # no such syscall exists
        jmp             return_from_syscall

sys_enter:
        pushl           $sys_leave
        pushl           %eax
        call            push_kernel_stack
        popl            %eax
        sti                                     # we want interrupts!
        jmp             *syscall_table(, %eax, 4)

sys_leave:
        cli
        pushl           %eax
        call            pop_kernel_stack
        popl            %eax
        jmp             return_from_syscall

# -----------------------------------------------------------------------------
# Interrupt Return / Task Switch

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

# -----------------------------------------------------------------------------
# Syscall Table
syscall_table:
  #include <i386/syscall_table.h>
_syscall_table_end:

NR_SYSCALLS = (_syscall_table_end - syscall_table) >> 2

# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Interrupt Thunk Routine Macros
#
# A thunk routine is a small non-returning function stub that is called by the
# CPU via the IDT when an interrupt is raised. These thunk routines push the
# interrupt vector or device IRQ number onto the stack, along with a 32-bit pad
# value of zero for interrupts that do not automatically push an error code,
# before jumping to the common interrupt handler.

##
# Intel exception thunk.
# Pushes a zero in lieu of an error code.
##
#define EXCEPTION_THUNK(thunk_name, vec_num)                                   \
.globl thunk_name;                                                             \
thunk_name:                                                                    \
        pushl   $0;                                                            \
        pushl   $vec_num;                                                      \
        jmp     recv_interrupt

##
# Intel exception thunk with error code already pushed by CPU.
##
#define EXCEPTION_THUNK_ERRCODE(thunk_name, vec_num)                           \
.globl thunk_name;                                                             \
thunk_name:                                                                    \
        pushl   $vec_num;                                                      \
        jmp     recv_interrupt

##
# Device IRQ thunk.
# Pushes the ones' compliment of the IRQ line number.
##
#define IRQ_THUNK(thunk_name, irq_num)                                         \
.globl thunk_name;                                                             \
thunk_name:                                                                    \
        pushl   $0;                                                            \
        pushl   $~irq_num;                                                     \
        jmp     recv_interrupt

# -----------------------------------------------------------------------------
# Interrupt Thunk Tables

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

# -----------------------------------------------------------------------------
# Interrupt Thunk Definitions

# exception types and their return behavior:
#     fault - re-excecutes faulting instruction upon return from handler
#      trap - executes instruction following trapping instruction
#     abort - severe error, interrupted program cannot be resumed

# 0-31                  processor exceptions                    classification
EXCEPTION_THUNK         (_except_00h, DIVIDE_ERROR)             # fault
EXCEPTION_THUNK         (_except_01h, DEBUG_EXCEPTION)          # fault or trap
EXCEPTION_THUNK         (_except_02h, NON_MASKABLE_INTERRUPT)   # n/a
EXCEPTION_THUNK         (_except_03h, BREAKPOINT_EXCEPTION)     # trap
EXCEPTION_THUNK         (_except_04h, OVERFLOW_EXCEPTION)       # trap
EXCEPTION_THUNK         (_except_05h, BOUND_RANGE_EXCEEDED)     # fault
EXCEPTION_THUNK         (_except_06h, INVALID_OPCODE)           # fault
EXCEPTION_THUNK         (_except_07h, DEVICE_NOT_AVAILABLE)     # fault
EXCEPTION_THUNK_ERRCODE (_except_08h, DOUBLE_FAULT)             # abort; error code is 0
EXCEPTION_THUNK         (_except_09h, SEGMENT_OVERRUN)          # abort; not used
EXCEPTION_THUNK_ERRCODE (_except_0Ah, INVALID_TSS)              # fault
EXCEPTION_THUNK_ERRCODE (_except_0Bh, SEGMENT_NOT_PRESENT)      # fault
EXCEPTION_THUNK_ERRCODE (_except_0Ch, STACK_FAULT)              # fault
EXCEPTION_THUNK_ERRCODE (_except_0Dh, GENERAL_PROTECTION_FAULT) # fault
EXCEPTION_THUNK_ERRCODE (_except_0Eh, PAGE_FAULT)               # fault
EXCEPTION_THUNK         (_except_0Fh, 0x0F)                     # (reserved)
EXCEPTION_THUNK         (_except_10h, MATH_FAULT)               # fault
EXCEPTION_THUNK_ERRCODE (_except_11h, ALIGNMENT_CHECK)          # fault
EXCEPTION_THUNK         (_except_12h, MACHINE_CHECK)            # abort
EXCEPTION_THUNK         (_except_13h, SIMD_FAULT)               # fault
EXCEPTION_THUNK         (_except_14h, 0x14)                     # fault
EXCEPTION_THUNK_ERRCODE (_except_15h, 0x15)                     # fault
EXCEPTION_THUNK         (_except_16h, 0x16)                     # (reserved)
EXCEPTION_THUNK         (_except_17h, 0x17)                     # (reserved)
EXCEPTION_THUNK         (_except_18h, 0x18)                     # (reserved)
EXCEPTION_THUNK         (_except_19h, 0x19)                     # (reserved)
EXCEPTION_THUNK         (_except_1Ah, 0x1A)                     # (reserved)
EXCEPTION_THUNK         (_except_1Bh, 0x1B)                     # (reserved)
EXCEPTION_THUNK         (_except_1Ch, 0x1C)                     # (reserved)
EXCEPTION_THUNK         (_except_1Dh, 0x1D)                     # (reserved)
EXCEPTION_THUNK         (_except_1Eh, 0x1E)                     # (reserved)
EXCEPTION_THUNK         (_except_1Fh, 0x1F)                     # (reserved)

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

# 48-79                 available

# 80                    system call
EXCEPTION_THUNK         (syscall_thunk, SYSCALL_VECTOR)

# 81-255                available
