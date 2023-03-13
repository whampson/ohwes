/*============================================================================*
 * Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                  *
 *                                                                            *
 * This file is part of the OHWES Operating System.                           *
 * OHWES is free software; you may redistribute it and/or modify it under the *
 * terms of the license agreement provided with this software.                *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: include/ohwes/interrupt.h                                         *
 * Created: December 14, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *                                                                            *
 * Generic interrupt handling.                                                *
 *============================================================================*/

#ifndef __INTERRUPT_H
#define __INTERRUPT_H

#define INT_EXCEPT          0x00
#define INT_IRQ             0x20
#define INT_SYSCALL         0x80

#ifndef __ASSEMBLER__
#include <stdint.h>

/**
 * The stack frame upon entry to an interrupt handler.
 */
struct iframe
{
    /* Interrupted process state.
       Pushed by common interrupt entry point. */
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t eax;

    /* Interrupt vector number.
       Exception number when an exception raised.
       One's Compliment of IRQ number when device IRQ raised.
       0x80 when executing system call. */
    uint32_t vec_num;

    /* Error code when an exception raised.
       Should be set to 0 when error code does not apply. */
    int32_t err_code;

    /* Hardware context.
       Pushed automatically by CPU when interrupt raised. */
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;   /* ESP and SS are only present when */
    uint32_t ss;    /* an interrupt causes a privilege level change. */
};

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

#endif /* __ASSEMBLER__ */

#endif /* __INTERRUPT _H */
