/* =============================================================================
 * Copyright (C) 2020-2024 Wes Hampson. All Rights Reserved.
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
 *         File: include/ohwes.h
 *      Created: January 7, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __OHWES_H
#define __OHWES_H

#include <stdio.h>

#define KERNEL_CS       0x10
#define KERNEL_DS       0x18
#define KERNEL_SS       KERNEL_DS
#define USER_CS         0x23
#define USER_DS         0x2B
#define USER_SS         USER_DS

#define SYS_EXIT        0           // syscall test

extern void halt(void);             // see entry.S

#define die()                       \
do {                                \
    for (;;);                       \
} while (0)

#define panic(...)                  \
do {                                \
    printf("panic: " __VA_ARGS__);  \
    die();                          \
} while (0)

#define _syscall0(func)             \
do {                                \
    __asm__ volatile (              \
        "int %0"                    \
        :                           \
        : "i"(INT_SYSCALL),         \
           "a"(func)                \
    );                              \
} while (0)

#define _syscall1(func,arg0)        \
do {                                \
    __asm__ volatile (              \
        "int %0"                    \
        :                           \
        : "i"(INT_SYSCALL),         \
          "a"(func),                \
          "b"(arg0)                 \
    );                              \
} while (0)

#endif // __OHWES_H
