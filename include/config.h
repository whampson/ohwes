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
 *      Created: April 15, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#define MIN_KB_REQUIRED                 639     // let's see how long this lasts!
#define PRINT_MEMORY_MAP                1
#define PRINT_IOCTL                     0
#define PRINT_PAGE_MAP                  1
#define E9_HACK                         1

#define IDT_BASE                        0x0800
#define PGDIR_BASE                      0x1000      //  0x1000 - 0x1FFF
#define PGTBL_BASE                      0x2000      //  0x2000 - 0x2FFF
#define MEMMAP_BASE                     0x3000      //  0x3000 - 0x3FFF
#define STACK_BASE                      0x7C00      // grows toward 0
#define STAGE2_BASE                     0x7E00

#define KERNEL_BASE                     0x10000     // 0x10000 - 0x9FBFF
#define KERNEL_ENTRY                    KERNEL_BASE

#define KERNEL_CS                       0x10
#define KERNEL_DS                       0x18
#define USER_CS                         0x23
#define USER_DS                         0x2B

//
// compiler stuff
//

#ifndef __GNUC__
#error "Please compile using GCC."
#else

/**
 * 'fastcall' calling convention.
 * Ensures the first two function arguments are passed through ECX and EDX
 * respectively. Remaining arguments are passed on the stack. Callee is
 * responsible for cleaning up the stack. ECX and EDX are not preserved by the
 * caller.
 */
#define __fastcall      __attribute__((fastcall))

/**
 * Case statement fall-through hint.
 */
#define __fallthrough   __attribute__((fallthrough))

/**
 * Pack a data structure; do not align or add padding between fields.
 */
#define __pack          __attribute__((packed))

/**
 * Align fields in a data structure to the nearest n bytes, where n is a power
 * of 2.
 */
#define __align(n)      __attribute__((aligned(n)))

/**
 * Indicate that a function does not return.
 */
#define __noreturn      __attribute__((noreturn))

#endif  // __GNUC__

#endif  // __CONFIG_H
