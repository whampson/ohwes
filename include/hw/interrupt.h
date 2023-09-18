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
    uint32_t vecNum;

    /**
     * Exception error code.
     * Zero for non-exception interrupts.
     */
    int32_t errCode;

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

_Static_assert(offsetof(struct iframe, ebx) == 0x00, "offsetof(struct iframe, ebx) == 0x00");
_Static_assert(offsetof(struct iframe, ecx) == 0x04, "offsetof(struct iframe, ecx) == 0x04");
_Static_assert(offsetof(struct iframe, edx) == 0x08, "offsetof(struct iframe, edx) == 0x08");
_Static_assert(offsetof(struct iframe, esi) == 0x0C, "offsetof(struct iframe, esi) == 0x0C");
_Static_assert(offsetof(struct iframe, edi) == 0x10, "offsetof(struct iframe, edi) == 0x10");
_Static_assert(offsetof(struct iframe, ebp) == 0x14, "offsetof(struct iframe, ebp) == 0x14");
_Static_assert(offsetof(struct iframe, eax) == 0x18, "offsetof(struct iframe, eax) == 0x18");
_Static_assert(offsetof(struct iframe, vecNum) == 0x1C, "offsetof(struct iframe, vecNum) == 0x1C");
_Static_assert(offsetof(struct iframe, errCode) == 0x20, "offsetof(struct iframe, errCode) == 0x20");
_Static_assert(offsetof(struct iframe, eip) == 0x24, "offsetof(struct iframe, eip) == 0x24");
_Static_assert(offsetof(struct iframe, cs) == 0x28, "offsetof(struct iframe, cs) == 0x28");
_Static_assert(offsetof(struct iframe, eflags) == 0x2C, "offsetof(struct iframe, eflags) == 0x2C");
_Static_assert(offsetof(struct iframe, esp) == 0x30, "offsetof(struct iframe, esp) == 0x30");
_Static_assert(offsetof(struct iframe, ss) == 0x34, "offsetof(struct iframe, ss) == 0x34");

typedef void (*IrqHandler)(void);

void IrqMask(int irqNum);
void IrqUnmask(int irqNum);
void IrqEnd(int irqNum);

bool IrqRegisterHandler(int irqNum, IrqHandler func);
void IrqUnregisterHandler(int irqNum);

typedef void (*IdtThunk)(void);

extern void Exception00h(void);
extern void Exception01h(void);
extern void Exception02h(void);
extern void Exception03h(void);
extern void Exception04h(void);
extern void Exception05h(void);
extern void Exception06h(void);
extern void Exception07h(void);
extern void Exception08h(void);
extern void Exception09h(void);
extern void Exception0Ah(void);
extern void Exception0Bh(void);
extern void Exception0Ch(void);
extern void Exception0Dh(void);
extern void Exception0Eh(void);
extern void Exception0Fh(void);
extern void Exception10h(void);
extern void Exception11h(void);
extern void Exception12h(void);
extern void Exception13h(void);
extern void Exception14h(void);
extern void Exception15h(void);
extern void Exception16h(void);
extern void Exception17h(void);
extern void Exception18h(void);
extern void Exception19h(void);
extern void Exception1Ah(void);
extern void Exception1Bh(void);
extern void Exception1Ch(void);
extern void Exception1Dh(void);
extern void Exception1Eh(void);
extern void Exception1Fh(void);

extern void Irq00h(void);
extern void Irq01h(void);
extern void Irq02h(void);
extern void Irq03h(void);
extern void Irq04h(void);
extern void Irq05h(void);
extern void Irq06h(void);
extern void Irq07h(void);
extern void Irq08h(void);
extern void Irq09h(void);
extern void Irq0Ah(void);
extern void Irq0Bh(void);
extern void Irq0Ch(void);
extern void Irq0Dh(void);
extern void Irq0Eh(void);
extern void Irq0Fh(void);

extern void Syscall(void);

extern void Interrupt(void);

#endif /* __ASSEMBLER__ */

#endif /* _INTERRUPT_H */
