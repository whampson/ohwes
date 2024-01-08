#include <interrupt.h>

.text
.code32

.globl kentry
kentry:
        cli
        call    kmain

.globl halt
halt:
        hlt
        jmp     halt

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
.globl do_interrupt
do_interrupt:
        subl            $IREGS_MAIN_SIZE, %esp
        movl            %ebx, IREGS_EBX(%esp)
        movl            %ecx, IREGS_ECX(%esp)
        movl            %edx, IREGS_EDX(%esp)
        movl            %esi, IREGS_ESI(%esp)
        movl            %edi, IREGS_EDI(%esp)
        movl            %ebp, IREGS_EBP(%esp)
        movl            %eax, IREGS_EAX(%esp)
        movw            %ds, %ax
        movw            %ax, IREGS_DS(%esp)
        movw            %es, %ax
        movw            %ax, IREGS_ES(%esp)
        movw            %fs, %ax
        movw            %ax, IREGS_FS(%esp)
        movw            %gs, %ax
        movw            %ax, IREGS_GS(%esp)

        movl            %esp, %ecx              # fastcall param (struct iregs *)
        movl            IREGS_VEC_NUM(%esp), %eax
        test            %eax, %eax
        js              irq_trampoline
        cmpl            $INT_SYSCALL, %eax
        je              syscall_trampoline
        jmp             interrupt_trampoline

        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        # noncontiguous execution
        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

end_interrupt:
end_irq:
        movl            IREGS_EAX(%esp), %eax   # syscall return value passed thru eax,
end_syscall:                                    # so we need to preserve it callee side
        movl            IREGS_EBP(%esp), %ebp
        movl            IREGS_EDI(%esp), %edi
        movl            IREGS_ESI(%esp), %esi
        movl            IREGS_EDX(%esp), %edx
        movl            IREGS_ECX(%esp), %ecx
        movl            IREGS_EBX(%esp), %ebx
        addl            $IREGS_EIP, %esp
        iret

interrupt_trampoline:
        pushl           $end_interrupt
        jmp             recv_interrupt

irq_trampoline:
        pushl           $end_irq
        jmp             recv_irq

syscall_trampoline:
        pushl           $end_syscall
        jmp             recv_syscall


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
# Exception thunk.
##
#define EXCEPTION_THUNK(thunk_name, vec_num)                                    \
.globl thunk_name;                                                              \
thunk_name:                                                                     \
        pushl   $0;                                                             \
        pushl   $vec_num;                                                       \
        jmp     do_interrupt

##
# Exception thunk w/ error code already pushed by processor.
##
#define EXCEPTION_THUNK_ERRCODE(thunk_name, vec_num)                            \
.globl thunk_name;                                                              \
thunk_name:                                                                     \
        pushl   $vec_num;                                                       \
        jmp     do_interrupt

##
# Device IRQ thunk.
##
#define THUNK_IRQ(thunk_name, vec_num)                                          \
.globl thunk_name;                                                              \
thunk_name:                                                                     \
        pushl   $0;                                                             \
        pushl   $~vec_num;                                                      \
        jmp     do_interrupt

##
# System call thunk.
##
#define THUNK_SYSCALL(thunk_name)                                               \
.globl thunk_name;                                                              \
thunk_name:                                                                     \
    pushl   $0;                                                                 \
    pushl   $INT_SYSCALL;                                                       \
    jmp     do_interrupt

#
#
#

# exception types:
#     fault - re-excecutes faulting instruction upon return from handler
#      trap - executes instruction following trapping instruction upon return from handler
#     abort - severe error, interrupted program cannot be resumed

# 0-31                  processor exceptions               classification
EXCEPTION_THUNK         (_thunk_except00h, EXCEPTION_DE)    # fault
EXCEPTION_THUNK         (_thunk_except01h, EXCEPTION_DB)    # fault/trap
EXCEPTION_THUNK         (_thunk_except02h, EXCEPTION_NMI)   # n/a
EXCEPTION_THUNK         (_thunk_except03h, EXCEPTION_BP)    # trap
EXCEPTION_THUNK         (_thunk_except04h, EXCEPTION_OF)    # trap
EXCEPTION_THUNK         (_thunk_except05h, EXCEPTION_BR)    # fault
EXCEPTION_THUNK         (_thunk_except06h, EXCEPTION_UD)    # fault
EXCEPTION_THUNK         (_thunk_except07h, EXCEPTION_NM)    # fault
EXCEPTION_THUNK_ERRCODE (_thunk_except08h, EXCEPTION_DF)    # abort; error code is 0
EXCEPTION_THUNK         (_thunk_except09h, 9)               # fault (not used, was Coprocessor Segment Overrun)
EXCEPTION_THUNK_ERRCODE (_thunk_except0Ah, EXCEPTION_TS)    # fault
EXCEPTION_THUNK_ERRCODE (_thunk_except0Bh, EXCEPTION_NP)    # fault
EXCEPTION_THUNK_ERRCODE (_thunk_except0Ch, EXCEPTION_SS)    # fault
EXCEPTION_THUNK_ERRCODE (_thunk_except0Dh, EXCEPTION_GP)    # fault
EXCEPTION_THUNK_ERRCODE (_thunk_except0Eh, EXCEPTION_PF)    # fault
EXCEPTION_THUNK         (_thunk_except0Fh, 15)              # (Intel reserved)
EXCEPTION_THUNK         (_thunk_except10h, EXCEPTION_MF)    # fault
EXCEPTION_THUNK_ERRCODE (_thunk_except11h, EXCEPTION_AC)    # fault, error code is 0
EXCEPTION_THUNK         (_thunk_except12h, EXCEPTION_MC)    # abort
EXCEPTION_THUNK         (_thunk_except13h, EXCEPTION_XM)    # fault
EXCEPTION_THUNK         (_thunk_except14h, EXCEPTION_VE)    # fault
EXCEPTION_THUNK_ERRCODE (_thunk_except15h, EXCEPTION_CP)    # fault
EXCEPTION_THUNK         (_thunk_except16h, 22)              # (Intel reserved)
EXCEPTION_THUNK         (_thunk_except17h, 23)              # (Intel reserved)
EXCEPTION_THUNK         (_thunk_except18h, 24)              # (Intel reserved)
EXCEPTION_THUNK         (_thunk_except19h, 25)              # (Intel reserved)
EXCEPTION_THUNK         (_thunk_except1Ah, 26)              # (Intel reserved)
EXCEPTION_THUNK         (_thunk_except1Bh, 27)              # (Intel reserved)
EXCEPTION_THUNK         (_thunk_except1Ch, 28)              # (Intel reserved)
EXCEPTION_THUNK         (_thunk_except1Dh, 29)              # (Intel reserved)
EXCEPTION_THUNK         (_thunk_except1Eh, 30)              # (Intel reserved)
EXCEPTION_THUNK         (_thunk_except1Fh, 31)              # (Intel reserved)

# 32-47                 device IRQs
THUNK_IRQ               (_thunk_irq00h, IRQ_TIMER)
THUNK_IRQ               (_thunk_irq01h, IRQ_KEYBOARD)
THUNK_IRQ               (_thunk_irq02h, IRQ_SLAVE)
THUNK_IRQ               (_thunk_irq03h, IRQ_COM2)
THUNK_IRQ               (_thunk_irq04h, IRQ_COM1)
THUNK_IRQ               (_thunk_irq05h, IRQ_LPT2)
THUNK_IRQ               (_thunk_irq06h, IRQ_FLOPPY)
THUNK_IRQ               (_thunk_irq07h, IRQ_LPT1)
THUNK_IRQ               (_thunk_irq08h, IRQ_RTC)
THUNK_IRQ               (_thunk_irq09h, IRQ_ACPI)
THUNK_IRQ               (_thunk_irq0Ah, IRQ_MISC1)
THUNK_IRQ               (_thunk_irq0Bh, IRQ_MISC2)
THUNK_IRQ               (_thunk_irq0Ch, IRQ_MOUSE)
THUNK_IRQ               (_thunk_irq0Dh, IRQ_COPOCESSOR)
THUNK_IRQ               (_thunk_irq0Eh, IRQ_ATA1)
THUNK_IRQ               (_thunk_irq0Fh, IRQ_ATA2)

# 48-79                 reserved

# 80                    system call
THUNK_SYSCALL           (_thunk_syscall)

# 81-255                reserved