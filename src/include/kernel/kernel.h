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
#include <kernel/task.h>

extern void pcspk_beep(int hz, int ms);                     // see timer.c
extern int _kprint(const char *fmt, ...);                   // print.c
extern __fastcall void _crash(struct iregs *regs);   // crash.c

#define kprint(...) _kprint(__VA_ARGS__)
#define beep(hz,ms) pcspk_beep(hz, ms)   // beep at hz for millis (nonblocking)
#define crash(regs) _crash(regs)

#define kprint_wrn(...)     \
    kprint("\n\e[1;33mwarn: "); kprint(__VA_ARGS__); kprint("\e[0m");
#define kprint_err(...)     \
    kprint("\n\e[31merror: ");  kprint(__VA_ARGS__); kprint("\e[0m");
// #define kprint_dbg()

#define panic(...) \
do { \
    kprint("\n\e[1;31mpanic: ");kprint(__VA_ARGS__); kprint("\e[0m"); \
    for (;;); \
} while (0)

#define panic_assert(x) \
do { \
    if (!(x)) { \
        panic(_ASSERT_STRING_FORMAT(x)); \
        for (;;); \
    } \
} while (0)

// #define kernel_task() get_task(0)

#endif  // !defined(__ASSEMBLER__) && defined(__KERNEL__)

#endif  // __KERNEL_H
