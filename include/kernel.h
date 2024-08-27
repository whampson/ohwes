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
 *         File: include/kernel.h
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
#define _TSS_SEGMENT                    0x38

#ifndef __KERNEL__
#error "Kernel-only defines live here!"
#endif

#ifndef __ASSEMBLER__

#include <assert.h>
#include <task.h>

extern int _kprint(const char *fmt, ...);
#define kprint(...) _kprint(__VA_ARGS__);

#define panic(...) \
do { \
    _kprint("\n\e[1;31mpanic: ");_kprint(__VA_ARGS__); _kprint("\e[0m"); \
    for(;;); \
} while (0)

#define panic_assert(x) \
do { \
    if (!(x)) { \
        panic(_ASSERT_STRING_FORMAT(x)); \
        for (;;); \
    } \
} while (0)

// #define kernel_task() get_task(0)

#ifdef DEBUG
extern int g_test_crash_kernel;
#endif


#define _IOC_CONSOLE    'c'     // VGA Console IOCTL code
#define _IOC_RTC        'r'     // RTC IOCTL code

#endif  // __ASSEMBLER__

#endif  // __KERNEL_H
