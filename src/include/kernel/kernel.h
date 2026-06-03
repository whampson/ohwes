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

#ifndef __KERNEL__
#error "Can't include this header outside of kernel mode!"
#endif

// x86 segment selectors (TODO: move to some x86 header)
#define KERNEL_LDT                      0x08
#define KERNEL_CS                       0x10
#define KERNEL_DS                       0x18
#define USER_CS                         0x23
#define USER_DS                         0x2B
#define KERNEL_TSS                      0x30
#define EMERG_TSS                       0x38

#ifndef __ASSEMBLER__

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <kernel/console.h>
#include <kernel/kprint.h>
#include <sys/ohwes.h>

#define __init  // TODO: put in special .init section or something

extern char g_panic_buf[BUFSIZ];
extern __fastcall __noreturn void _panic(const char *reason);

#define panic(...) \
do { \
    snprintf(g_panic_buf, sizeof(g_panic_buf), __VA_ARGS__); \
    _panic(g_panic_buf); \
} while (0)

// beep at hz for millis;
//  interrupts must be ON or it will beep/block forever!
extern void beep(int hz, int ms, bool block);

// get the amount of time the system has been up and running, in microseconds
extern uint64_t get_uptime(void);

// TODO: verify/test these!!
#define PHYSICAL_ADDR(v)    (((uintptr_t) (v) >= KERNEL_VA) ? ((uintptr_t) (v) - KERNEL_VA) : (uintptr_t) (v))
#define KERNEL_ADDR(p)      (((uintptr_t) (p) >= -KERNEL_VA)  ? (uintptr_t) (p) : ((uintptr_t) (p) + KERNEL_VA))

// convert integer to pointer type, useful for %p
#define _P(addr) ((void *) (addr))

#endif  // ndef __ASSEMBLER__

#endif  // __KERNEL_H
