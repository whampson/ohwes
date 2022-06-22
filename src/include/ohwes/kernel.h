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

#define warn(...)                                                           \
do {                                                                        \
    kprintf("Warning: " __VA_ARGS__);                                       \
} while (0)

#define panic(...)                                                          \
do {                                                                        \
    kprintf("PANIC: " __VA_ARGS__);                                         \
    for (;;);                                                               \
} while (0)

#define halt()                                                              \
do {                                                                        \
    __asm__ volatile ("_die%=: hlt; jmp _die%=" : );                        \
} while (0)

#endif /* __KERNEL_H */
