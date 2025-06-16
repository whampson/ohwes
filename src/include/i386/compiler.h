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
 *         File: include/compiler.h
 *      Created: July 19, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __COMPILER_H
#define __COMPILER_H

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

#endif  // __COMPILER_H
