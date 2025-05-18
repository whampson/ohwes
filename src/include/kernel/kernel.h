/* =============================================================================
 * Copyright (C) 2020-2025 Wes Hampson. All Rights Reserved.
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
 *         File: src/include/kernel/kernel.h
 *      Created: April 15, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __KERNEL_H
#define __KERNEL_H

// x86 segment selectors (TODO: move to some x86 header)
#define KERNEL_CS                       0x10
#define KERNEL_DS                       0x18
#define USER_CS                         0x23
#define USER_DS                         0x2B
#define _LDT_SEGMENT                    0x30
#define _TSS0_SEGMENT                   0x38
#define _TSS1_SEGMENT                   0x40

#ifndef __KERNEL__
#error "Kernel-only defines live here!"
#endif

#if !defined(__ASSEMBLER__) && defined(__KERNEL__)

#include <assert.h>
#include <i386/interrupt.h>
#include <kernel/config.h>
#include <kernel/console.h>
#include <kernel/task.h>

#define __setup __attribute__((section(".setup")))

#define ALERT_FREQ  1725
#define ALERT_TIME   100

// TODO: use this on kprint and panic to sanitize format string
//  __attribute__((format(printf, 1, 2)));

// printf to console
extern int kprint(const char *fmt, ...);

// halt and catch fire
extern __noreturn void panic(const char *fmt, ...);

// beep at hz for millis (nonblocking);
//  interrupts must be ON or it will beep forever!
extern void beep(int hz, int ms);

// print alert message and beep, then continue;
//  interrupts must be ON or this will beep forever!
#define alert(...) \
do { \
    kprint("\e[1;33malert: " __VA_ARGS__); kprint("\e[0m"); \
    beep(ALERT_FREQ, ALERT_TIME); \
} while (0)

// zero memory
#define zeromem(p,n)    memset(p, 0, n)

// TODO: verify/test these!!
#define PHYSICAL_ADDR(v)    (((uintptr_t) (v) >= KERNEL_VA) ? ((uintptr_t) (v) - KERNEL_VA) : (uintptr_t) (v))
#define KERNEL_ADDR(p)      (((uintptr_t) (p) >= -KERNEL_VA)  ? (uintptr_t) (p) : ((uintptr_t) (p) + KERNEL_VA))

#endif  // !defined(__ASSEMBLER__) && defined(__KERNEL__)
#endif  // __KERNEL_H
