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
 *         File: compiler.h
 *      Created: December 30, 2020
 *       Author: Wes Hampson
 *
 * Compiler-specific stuff.
 * =============================================================================
 */

#ifndef __COMPILER_H
#define __COMPILER_H

#ifdef __GNUC__     // GCC

#include <stdint.h>

/**
 * 'fastcall' calling convention.
 * Passes the first two arguments through ECX and EDX respectively.
 */
#define __fastcall      __attribute__((fastcall))

/**
 * 'syscall' calling convention.
 * Ensures parameters are always retrieved from the stack.
 */
#define __syscall       __attribute__((regparm(0)))

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
 * 64-bit division functions.
 */

int64_t __moddi3(int64_t, int64_t);
int64_t __divdi3(int64_t, int64_t);
int64_t __divmoddi4(int64_t, int64_t, int64_t *);

uint64_t __umoddi3(uint64_t, uint64_t);
uint64_t __udivdi3(uint64_t, uint64_t);
uint64_t __udivmoddi4(uint64_t, uint64_t, uint64_t *);

#else
#error "Please compile using GCC."
#endif  /* __GNUC__ */

#endif /* __COMPILER_H */
