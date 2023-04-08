/* =============================================================================
 * Copyright (C) 2020-2023 Wes Hampson. All Rights Reserved.
 *
 * This file is part of the OH-WES Operating System.
 * OH-WES is free software; you may redistribute it and/or modify it under the
 * terms of the GNU GPLv2. See the LICENSE file in the root of this repository.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * -----------------------------------------------------------------------------
 *         File: include/hw/interrupt.h
 *      Created: December 14, 2020
 *       Author: Wes Hampson
 * =============================================================================
 */

/* WARNING: The functions defined in this file are tightly coupled to x86. */

#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#define INT_EXCEPTION       0x00    /* Processor exception vector number. */
#define INT_IRQ             0x20    /* Device IRQ interrupt vector number. */
#define INT_SYSCALL         0x80    /* System call interrupt vector number. */

#define EXCEPTION_DE        0x00    /* Divide Error */
#define EXCEPTION_DB        0x01    /* Debug Exception */
#define EXCEPTION_NMI       0x02    /* Non-Maskable Interrupt */
#define EXCEPTION_BP        0x03    /* Breakpoint */
#define EXCEPTION_OF        0x04    /* Overflow */
#define EXCEPTION_BR        0x05    /* BOUND Range Exceeded */
#define EXCEPTION_UD        0x06    /* Invalid Opcode */
#define EXCEPTION_NM        0x07    /* Device Not Available */
#define EXCEPTION_DF        0x08    /* Double Fault */
#define EXCEPTION_TS        0x0A    /* Invalid TSS */
#define EXCEPTION_NP        0x0B    /* Segment Not Present */
#define EXCEPTION_SS        0x0C    /* Stack Fault */
#define EXCEPTION_GP        0x0D    /* General Protection Fault */
#define EXCEPTION_PF        0x0E    /* Page Fault */
#define EXCEPTION_MF        0x10    /* Math Fault (x87 FPU Floating-Point Error) */
#define EXCEPTION_AC        0x11    /* Alignment Check */
#define EXCEPTION_MC        0x12    /* Machine Check */
#define EXCEPTION_XM        0x13    /* SIMD Floating-Point Exception */
#define EXCEPTION_VE        0x14    /* Virtualization Exception */
#define EXCEPTION_CP        0x15    /* Control Protection Exception */
#define NUM_EXCEPTION       32      /* Number of processor exception vectors. */

#define IRQ_TIMER           0
#define IRQ_KEYBOARD        1
#define IRQ_SLAVE_PIC       2
#define IRQ_COM2            3
#define IRQ_COM1            4
#define IRQ_LPT2            5
#define IRQ_FLOPPY          6
#define IRQ_LPT1            7
#define IRQ_RTC             8
#define IRQ_ACPI            9
#define IRQ_10              10
#define IRQ_11              11
#define IRQ_MOUSE           12
#define IRQ_COPOCESSOR      13
#define IRQ_ATA1            14
#define IRQ_ATA2            15
#define NUM_IRQ             16

#ifndef __ASSEMBLER__
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * The following functions are defined as macros to ensure assembly instructions
 * are injected in-line.
 */

/**
 * Clears the interrupt flag.
 */
#define cli()               \
__asm__ volatile (          \
    "cli"                   \
    :                       \
    :                       \
    : "cc"                  \
)

/**
 * Sets the interrupt flag.
 */
#define sti()               \
__asm__ volatile (          \
    "sti"                   \
    :                       \
    :                       \
    : "cc"                  \
)

/**
 * Backs up the EFLAGS register, then clears the interrupt flag.
 */
#define cli_save(flags)     \
__asm__ volatile (          \
    "                       \n\
    pushfl                  \n\
    popl %0                 \n\
    cli                     \n\
    "                       \
    : "=r"(flags)           \
    :                       \
    : "memory", "cc"        \
)

/**
 * Restores the EFLAGS register.
 * If interrupts were previously enabled, this will also restore interrupts.
 */
#define restore_flags(flags)\
__asm__ volatile (          \
    "                       \n\
    push %0                 \n\
    popfl                   \n\
    "                       \
    :                       \
    : "r"(flags)            \
    : "memory", "cc"        \
)

/**
 * The stack frame upon entry to an interrupt handler.
 */
struct iframe
{
    /**
     * Interrupted process state.
     * Pushed by common interrupt handler.
     */

    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t eax;

    /**
     * Interrupt vector number.
     *
     * INT_EXCEPTION: Exception number.
     * INT_IRQ: One's Compliment of device IRQ number.
     * INT_SYSCALL: 0x80 when executing system call.
     */
    uint32_t vec_num;

    /**
     * Exception error code.
     * Zero for non-exception interrupts.
     */
    int32_t err_code;

    /**
     * Hardware context.
     * Pushed automatically by CPU when interrupt is raised.
     */

    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;   /* ESP and SS are only present when */
    uint32_t ss;    /*   an interrupt causes a privilege level change. */
};

_Static_assert(offsetof(struct iframe, ebx) == 0x00, "offsetof(struct iframe, ebx)");
_Static_assert(offsetof(struct iframe, ecx) == 0x04, "offsetof(struct iframe, ecx)");
_Static_assert(offsetof(struct iframe, edx) == 0x08, "offsetof(struct iframe, edx)");
_Static_assert(offsetof(struct iframe, esi) == 0x0C, "offsetof(struct iframe, esi)");
_Static_assert(offsetof(struct iframe, edi) == 0x10, "offsetof(struct iframe, edi)");
_Static_assert(offsetof(struct iframe, ebp) == 0x14, "offsetof(struct iframe, ebp)");
_Static_assert(offsetof(struct iframe, eax) == 0x18, "offsetof(struct iframe, eax)");
_Static_assert(offsetof(struct iframe, vec_num) == 0x1C, "offsetof(struct iframe, vec_num)");
_Static_assert(offsetof(struct iframe, err_code) == 0x20, "offsetof(struct iframe, err_code)");
_Static_assert(offsetof(struct iframe, eip) == 0x24, "offsetof(struct iframe, eip)");
_Static_assert(offsetof(struct iframe, cs) == 0x28, "offsetof(struct iframe, cs)");
_Static_assert(offsetof(struct iframe, eflags) == 0x2C, "offsetof(struct iframe, eflags)");
_Static_assert(offsetof(struct iframe, esp) == 0x30, "offsetof(struct iframe, esp)");
_Static_assert(offsetof(struct iframe, ss) == 0x34, "offsetof(struct iframe, ss)");


typedef void (*irq_handler)(void);

void irq_mask(int irq_num);
void irq_unmask(int irq_num);
void irq_eoi(int irq_num);

bool irq_register_handler(int irq_num, irq_handler func);
void irq_unregister_handler(int irq_num);


typedef void (*idt_thunk)(void);

extern void _thunk_exception_00h(void);
extern void _thunk_exception_01h(void);
extern void _thunk_exception_02h(void);
extern void _thunk_exception_03h(void);
extern void _thunk_exception_04h(void);
extern void _thunk_exception_05h(void);
extern void _thunk_exception_06h(void);
extern void _thunk_exception_07h(void);
extern void _thunk_exception_08h(void);
extern void _thunk_exception_09h(void);
extern void _thunk_exception_0ah(void);
extern void _thunk_exception_0bh(void);
extern void _thunk_exception_0ch(void);
extern void _thunk_exception_0dh(void);
extern void _thunk_exception_0eh(void);
extern void _thunk_exception_0fh(void);
extern void _thunk_exception_10h(void);
extern void _thunk_exception_11h(void);
extern void _thunk_exception_12h(void);
extern void _thunk_exception_13h(void);
extern void _thunk_exception_14h(void);
extern void _thunk_exception_15h(void);
extern void _thunk_exception_16h(void);
extern void _thunk_exception_17h(void);
extern void _thunk_exception_18h(void);
extern void _thunk_exception_19h(void);
extern void _thunk_exception_1ah(void);
extern void _thunk_exception_1bh(void);
extern void _thunk_exception_1ch(void);
extern void _thunk_exception_1dh(void);
extern void _thunk_exception_1eh(void);
extern void _thunk_exception_1fh(void);

extern void _thunk_irq_00h(void);
extern void _thunk_irq_01h(void);
extern void _thunk_irq_02h(void);
extern void _thunk_irq_03h(void);
extern void _thunk_irq_04h(void);
extern void _thunk_irq_05h(void);
extern void _thunk_irq_06h(void);
extern void _thunk_irq_07h(void);
extern void _thunk_irq_08h(void);
extern void _thunk_irq_09h(void);
extern void _thunk_irq_0ah(void);
extern void _thunk_irq_0bh(void);
extern void _thunk_irq_0ch(void);
extern void _thunk_irq_0dh(void);
extern void _thunk_irq_0eh(void);
extern void _thunk_irq_0fh(void);

extern void _thunk_syscall(void);

extern void _thunk_generic_interrupt(void);

#endif /* __ASSEMBLER__ */

#endif /* _INTERRUPT_H */
