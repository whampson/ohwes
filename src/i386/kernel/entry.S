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
#include <i386/x86.h>
#include <kernel/irq.h>
#include <kernel/task.h>

##
# -----------------------------------------------------------------------------
# recv_interrupt
# -----------------------------------------------------------------------------
# Common Interrupt Handler
#
# The Common Interrupt Handler serves as the main entry point for all CPU
# interrupts and software traps. Its purpose is to save interrupted process'
# state, determine the type of interrupt that occurred, set up the return stack,
# then pass control on to the appropriate interrupt service routine. The process
# state is stored on the stack in an 'iregs' structure, to be restored upon
# completion of the routine.
#
# Upon entry to this function, several registers will already be present on the
# stack. The CPU itself pushes some registers onto the stack automatically when
# an interrupt occurs, but the number of registers pushed depends on whether the
# interrupted process was executing as user or supervisor (kernel). In addition,
# an exception error code and interrupt vector number are also pushed to the
# stack, regardless of the type of interrupt that occurred. The following
# routine will push the remaining context registers onto the stack to create the
# iregs structure, then pass a pointer to that structure to the appropriate
# interrupt servive routine. Because the number of registers iniitally pushed to
# the stack by the CPU is variable, this routine will re-push those registers to
# ensure the iregs structure is always fully initalized when it is receieved by
# the interrupt service routine.
#
# Registers are pushed onto the stack as follows:
# sp1 -> ebx       pushed by recv_interrupt; syscall param 0
#        ecx       pushed by recv_interrupt; syscall param 1
#        edx       pushed by recv_interrupt; syscall param 2
#        esi       pushed by recv_interrupt; syscall param 3
#        edi       pushed by recv_interrupt; syscall param 4
#        ebp       pushed by recv_interrupt
#        eax       pushed by recv_interrupt; syscall number / return value
#        ds        pushed by recv_interrupt
#        es        pushed by recv_interrupt
#        fs        pushed by recv_interrupt
#        gs        pushed by recv_interrupt
#        vec       pushed by recv_interrupt
#        err       pushed by recv_interrupt
#        eip       pushed by recv_interrupt
#        cs        pushed by recv_interrupt
#        eflags    pushed by recv_interrupt
#        esp       pushed by recv_interrupt
#        ss        pushed by recv_interrupt
# sp0 -> vec       pushed by thunk; interrupt vector number, 0x80 for syscall
#        err       pushed by cpu or thunk depending on exception
#        eip       pushed by cpu
#        cs        pushed by cpu
#        eflags    pushed by cpu
#        esp       pushed by cpu; only present upon privilege level change
#        ss        pushed by cpu; only present upon privilege level change
#
# In the above diagram, 'sp0' refers to the stack pointer upon entry to this
# routine, while 'sp1' refers to the stack pointer after all registers have been
# pushed.

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
# bitwise NOT of the vector number. Device IRQs are expected by the PIC to be
# acknowledged with an End of Interrupt (EOI) message upon completion of the
# handler. Like exceptions, device IRQs do not return any meaningful value to
# the user, but unlike exceptions they are always expected to return.
#
# A System Call is a programmatic call into the kernel (trap) in order to
# perform some privileged operation, such as reading/writing data to a device or
# allocating memory. System calls are invoked using the 'int' instruction and
# always use the interrupt vector number 0x80. The requested kernel function is
# specified as a 32-bit number in EAX, while the parameters to the function are
# passed via EBX, ECX, EDX, ESI, and EDI registers (in that order with respect
# to a C-style function signature) for a total of up to five 32-bit parameters.
# Our common interrupt handler places these parameters on the stack while it's
# saving program state, allowing us to simply call the requested function. To
# ensure that system calls with C linkage retrieve their parameters from the
# stack only, we declare the kernel side of a system call using the __syscall
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
        subl            $SIZEOF_IREGS, %esp
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
        MEM2MEM         SIZEOF_IREGS+IRET_VEC(%esp), IREGS_VEC(%esp)
        MEM2MEM         SIZEOF_IREGS+IRET_ERR(%esp), IREGS_ERR(%esp)
        MEM2MEM         SIZEOF_IREGS+IRET_EFLAGS(%esp), IREGS_EFLAGS(%esp)
        MEM2MEM         SIZEOF_IREGS+IRET_EIP(%esp), IREGS_EIP(%esp)
        MEM2MEM         SIZEOF_IREGS+IRET_CS(%esp), IREGS_CS(%esp)
        andl            $0x3, %eax
        test            %eax, %eax              # priv level check
        jz              _store_kernel_stack     # if was ring0, store regs
        MEM2MEM         SIZEOF_IREGS+IRET_ESP(%esp), IREGS_ESP(%esp)
        MEM2MEM         SIZEOF_IREGS+IRET_SS(%esp), IREGS_SS(%esp)
        jmp             handle_interrupt

_store_kernel_stack:
        movl            %esp, IREGS_ESP(%esp)
        STORE_SEGMENT   %ss, IREGS_SS(%esp)
        jmp             handle_interrupt

##
# -----------------------------------------------------------------------------
# recv_interrupt_tss
# -----------------------------------------------------------------------------
# Like 'recv_interrupt', except called via a TSS, so the interrupted program
# state is stored in the 'prev' link within the active TSS. This routine crafts
# an iregs structure from the saved TSS and funnels control to handle_interrupt.
##
recv_interrupt_tss:
        cli
        call            get_curr_tss
        pushl           TSS_PREV(%eax)
        call            get_tss
        addl            $4, %esp
        movl            %eax, %ebx
        subl            $SIZEOF_IREGS, %esp
        MEM2MEM         TSS_EBX(%ebx), IREGS_EBX(%esp)
        MEM2MEM         TSS_ECX(%ebx), IREGS_ECX(%esp)
        MEM2MEM         TSS_EDX(%ebx), IREGS_EDX(%esp)
        MEM2MEM         TSS_ESI(%ebx), IREGS_ESI(%esp)
        MEM2MEM         TSS_EDI(%ebx), IREGS_EDI(%esp)
        MEM2MEM         TSS_EBP(%ebx), IREGS_EBP(%esp)
        MEM2MEM         TSS_EAX(%ebx), IREGS_EAX(%esp)
        MEM2MEM         TSS_EIP(%ebx), IREGS_EIP(%esp)
        MEM2MEM         TSS_ESP(%ebx), IREGS_ESP(%esp)
        MEM2MEM         TSS_EFLAGS(%ebx), IREGS_EFLAGS(%esp)
        MEM2MEM         TSS_CS(%ebx), IREGS_CS(%esp)
        MEM2MEM         TSS_DS(%ebx), IREGS_DS(%esp)
        MEM2MEM         TSS_ES(%ebx), IREGS_ES(%esp)
        MEM2MEM         TSS_FS(%ebx), IREGS_FS(%esp)
        MEM2MEM         TSS_GS(%ebx), IREGS_GS(%esp)
        MEM2MEM         TSS_SS(%ebx), IREGS_SS(%esp)
        MEM2MEM         SIZEOF_IREGS+IRET_VEC(%esp), IREGS_VEC(%esp)
        MEM2MEM         SIZEOF_IREGS+IRET_ERR(%esp), IREGS_ERR(%esp)
        jmp             handle_interrupt

# -----------------------------------------------------------------------------

handle_interrupt:
        call            current_task
        movl            %esp, TASK_IREGS(%eax)  # store iregs in current task
        movl            %esp, %ecx              # ecx = fastcall param
        movl            IREGS_VEC(%esp), %eax
        test            %eax, %eax
        js              irq_trampoline
        cmpl            $SYSCALL_VECTOR, %eax
        je              syscall_trampoline
        jmp             exception_trampoline

# -----------------------------------------------------------------------------

irq_trampoline:
        pushl           $_return_from_irq
        jmp             handle_irq

exception_trampoline:
        pushl           $_return_from_exception
        jmp             handle_exception

syscall_trampoline:
        movl            IREGS_EAX(%ecx), %eax   # eax = syscall number
        cmpl            $NR_SYSCALLS, %eax
        jbe             sys_enter
        movl            $-ENOSYS, %eax
        jmp             _return_from_syscall

sys_enter:
        pushl           $sys_leave
        sti
        jmp             *syscall_table(, %eax, 4)

sys_leave:
        cli
        jmp             _return_from_syscall

# -----------------------------------------------------------------------------
# Interrupt Return / Task Switch

# context switch
.globl switch_context
switch_context:
        cli
        movl            %ecx, %esp              # ecx = iregs ptr
_return_from_exception:
_return_from_irq:
        movl            IREGS_EAX(%esp), %eax
_return_from_syscall:
        movl            IREGS_EBP(%esp), %ebp
        movl            IREGS_EDI(%esp), %edi
        movl            IREGS_ESI(%esp), %esi
        movl            IREGS_EDX(%esp), %edx
        movl            IREGS_ECX(%esp), %ecx
        movl            IREGS_EBX(%esp), %ebx
        movl            %eax, IREGS_ERR(%esp)   # reuse error code slot to
        LOAD_SEGMENT    IREGS_DS(%esp), %ds     #  temporarily store eax; used
        LOAD_SEGMENT    IREGS_ES(%esp), %es     #  by these macros to restore
        LOAD_SEGMENT    IREGS_FS(%esp), %fs     #  segment registers
        LOAD_SEGMENT    IREGS_GS(%esp), %gs
        MEM2MEM         IREGS_EFLAGS(%esp), SIZEOF_IREGS+IRET_EFLAGS(%esp)
        MEM2MEM         IREGS_EIP(%esp), SIZEOF_IREGS+IRET_EIP(%esp)
        MEM2MEM         IREGS_CS(%esp), SIZEOF_IREGS+IRET_CS(%esp)
        andl            $0x03, %eax
        test            %eax, %eax
        jz              _return_from_interrupt
        MEM2MEM         IREGS_ESP(%esp), SIZEOF_IREGS+IRET_ESP(%esp)
        MEM2MEM         IREGS_SS(%esp), SIZEOF_IREGS+IRET_SS(%esp)
_return_from_interrupt:
        movl            IREGS_ERR(%esp), %eax   # restore eax
        addl            $SIZEOF_IREGS+IRET_EIP, %esp
        iret

# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Syscall Table

syscall_table:
  #include <i386/syscall_table.h>
_syscall_table_end:

NR_SYSCALLS = (_syscall_table_end - syscall_table) >> 2

# -----------------------------------------------------------------------------
# Interrupt Thunk Tables

.globl _exception_thunks
_exception_thunks:
        .long           _except_00h, _except_01h, _except_02h, _except_03h
        .long           _except_04h, _except_05h, _except_06h, _except_07h
        .long           _except_08h, _except_09h, _except_0Ah, _except_0Bh
        .long           _except_0Ch, _except_0Dh, _except_0Eh, _except_0Fh
        .long           _except_10h, _except_11h, _except_12h, _except_13h
        .long           _except_14h, _except_15h, _except_16h, _except_17h
        .long           _except_18h, _except_19h, _except_1Ah, _except_19h
        .long           _except_1Ch, _except_1Ch, _except_1Eh, _except_1Fh

.globl _irq_thunks
_irq_thunks:
        .long           _irq_00h, _irq_01h, _irq_02h, _irq_03h
        .long           _irq_04h, _irq_05h, _irq_06h, _irq_07h
        .long           _irq_08h, _irq_09h, _irq_0Ah, _irq_0Bh
        .long           _irq_0Ch, _irq_0Dh, _irq_0Eh, _irq_0Fh

# -----------------------------------------------------------------------------
# Interrupt Thunk Routine Macros
#
# A thunk routine is a small non-returning function stub that is called by the
# CPU via the IDT when an interrupt is raised. These thunk routines push the
# interrupt vector or device IRQ number onto the stack, along with a 32-bit pad
# value of zero for interrupts that do not automatically push an error code,
# before jumping to the common interrupt handler.

#define IDT_THUNK(thunk_name, handler, vec_num)                                \
.globl thunk_name;                                                             \
thunk_name:                                                                    \
        pushl   $0;                                                            \
        pushl   $vec_num;                                                      \
        jmp     handler

#define IDT_THUNK_ERR(thunk_name, handler, vec_num)                            \
.globl thunk_name;                                                             \
thunk_name:                                                                    \
        pushl   $vec_num;                                                      \
        jmp     handler

# -----------------------------------------------------------------------------
# Interrupt Thunk Definitions

# exception types and their return behavior:
#     fault - re-excecutes faulting instruction upon return from handler
#      trap - executes instruction following trapping instruction
#     abort - severe error, interrupted program cannot be resumed

# 0-31                  processor exceptions                        classification
IDT_THUNK       (_except_00h, recv_interrupt, DIVIDE_ERROR)         # fault
IDT_THUNK       (_except_01h, recv_interrupt, DEBUG_EXCEPTION)      # fault or trap
IDT_THUNK       (_except_02h, recv_interrupt, NMI_INTERRUPT)        # n/a
IDT_THUNK       (_except_03h, recv_interrupt, BREAKPOINT)           # trap
IDT_THUNK       (_except_04h, recv_interrupt, OVERFLOW_EXCEPTION)   # trap
IDT_THUNK       (_except_05h, recv_interrupt, BOUND_RANGE_EXCEEDED) # fault
IDT_THUNK       (_except_06h, recv_interrupt, INVALID_OPCODE)       # fault
IDT_THUNK       (_except_07h, recv_interrupt, DEVICE_NOT_AVAILABLE) # fault
IDT_THUNK_ERR   (_except_08h, recv_interrupt_tss, DOUBLE_FAULT)     # abort; err=0
IDT_THUNK       (_except_09h, recv_interrupt, SEGMENT_OVERRUN)      # abort; unused
IDT_THUNK_ERR   (_except_0Ah, recv_interrupt, INVALID_TSS)          # fault
IDT_THUNK_ERR   (_except_0Bh, recv_interrupt, SEGMENT_NOT_PRESENT)  # fault
IDT_THUNK_ERR   (_except_0Ch, recv_interrupt, STACK_FAULT)          # fault
IDT_THUNK_ERR   (_except_0Dh, recv_interrupt, PROTECTION_FAULT)     # fault
IDT_THUNK_ERR   (_except_0Eh, recv_interrupt, PAGE_FAULT)           # fault
IDT_THUNK       (_except_0Fh, recv_interrupt, 0x0F)                 # (reserved)
IDT_THUNK       (_except_10h, recv_interrupt, MATH_FAULT)           # fault
IDT_THUNK_ERR   (_except_11h, recv_interrupt, ALIGNMENT_CHECK)      # fault
IDT_THUNK       (_except_12h, recv_interrupt, MACHINE_CHECK)        # abort
IDT_THUNK       (_except_13h, recv_interrupt, SIMD_FAULT)           # fault
IDT_THUNK       (_except_14h, recv_interrupt, 0x14)                 # fault
IDT_THUNK_ERR   (_except_15h, recv_interrupt, 0x15)                 # fault
IDT_THUNK       (_except_16h, recv_interrupt, 0x16)                 # (reserved)
IDT_THUNK       (_except_17h, recv_interrupt, 0x17)                 # (reserved)
IDT_THUNK       (_except_18h, recv_interrupt, 0x18)                 # (reserved)
IDT_THUNK       (_except_19h, recv_interrupt, 0x19)                 # (reserved)
IDT_THUNK       (_except_1Ah, recv_interrupt, 0x1A)                 # (reserved)
IDT_THUNK       (_except_1Bh, recv_interrupt, 0x1B)                 # (reserved)
IDT_THUNK       (_except_1Ch, recv_interrupt, 0x1C)                 # (reserved)
IDT_THUNK       (_except_1Dh, recv_interrupt, 0x1D)                 # (reserved)
IDT_THUNK       (_except_1Eh, recv_interrupt, 0x1E)                 # (reserved)
IDT_THUNK       (_except_1Fh, recv_interrupt, 0x1F)                     # (reserved)

# 32-47         device IRQs
IDT_THUNK       (_irq_00h, recv_interrupt, ~IRQ_TIMER)
IDT_THUNK       (_irq_01h, recv_interrupt, ~IRQ_KEYBOARD)
IDT_THUNK       (_irq_02h, recv_interrupt, ~IRQ_SLAVE)
IDT_THUNK       (_irq_03h, recv_interrupt, ~IRQ_COM2)
IDT_THUNK       (_irq_04h, recv_interrupt, ~IRQ_COM1)
IDT_THUNK       (_irq_05h, recv_interrupt, ~IRQ_LPT2)
IDT_THUNK       (_irq_06h, recv_interrupt, ~IRQ_FLOPPY)
IDT_THUNK       (_irq_07h, recv_interrupt, ~IRQ_LPT1)
IDT_THUNK       (_irq_08h, recv_interrupt, ~IRQ_RTC)
IDT_THUNK       (_irq_09h, recv_interrupt, ~IRQ_ACPI)
IDT_THUNK       (_irq_0Ah, recv_interrupt, ~IRQ_MISC1)
IDT_THUNK       (_irq_0Bh, recv_interrupt, ~IRQ_MISC2)
IDT_THUNK       (_irq_0Ch, recv_interrupt, ~IRQ_MOUSE)
IDT_THUNK       (_irq_0Dh, recv_interrupt, ~IRQ_COPOCESSOR)
IDT_THUNK       (_irq_0Eh, recv_interrupt, ~IRQ_ATA1)
IDT_THUNK       (_irq_0Fh, recv_interrupt, ~IRQ_ATA2)

# 48-79         available

# 80            system call
IDT_THUNK       (_syscall_thunk, recv_interrupt, SYSCALL_VECTOR)

# 81-255        available
