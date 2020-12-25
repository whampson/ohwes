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
 *    File: include/ohwes/kernel.h                                            *
 * Created: December 13, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#ifndef __KERNEL_H
#define __KERNEL_H

#include <stdio.h>
#include <ohwes/console.h>

/* Privilege Levels */
#define KERNEL_PL   0                       /* Kernel Privilege Level */
#define USER_PL     3                       /* User Privilege Level */

/* Segment Selectors */
#define KERNEL_CS   (0x10|KERNEL_PL)        /* Kernel Code Segment */
#define KERNEL_DS   (0x18|KERNEL_PL)        /* Kernel Data Segment */
#define USER_CS     (0x20|USER_PL)          /* User-space Code Segment */
#define USER_DS     (0x28|USER_PL)          /* User-space Data Segment */
#define LDT         (0x30|KERNEL_PL)        /* LDT Segment */
#define TSS         (0x38|KERNEL_PL)        /* TSS Segment */

/**
 * Prints a message to the kernel console.
 * We use a separate function in case we want to
 * divorce ourselves from printf() and log the
 * kernel output one day.
 */
#define kprintf(...)    printf(__VA_ARGS__)

/**
 * Uh oh, something bad happened!
 * Prints a message then halts the system.
 */
#define panic(x)                            \
do {                                        \
    kprintf("KERNEL PANIC: " x);            \
    for (;;);                               \
} while (0)

/* main.c */
void gdt_init(void);
void ldt_init(void);
void tss_init(void);

/* console.c */
void con_init(void);

/* memory.c */
void mem_init(void);

/* interrupt.c */
void idt_init(void);

/* irq.c */
void irq_init(void);

/* i8042.c */
void ps2_init(void);

#endif /* __KERNEL_H */
