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
 *    File: include/ohwes/compiler.h                                          *
 * Created: December 30, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#ifndef __COMPILER_H
#define __COMPILER_H

#ifdef __GNUC__

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

#else
#error "Please compile using GCC."
#endif  /* __GNUC__ */

#endif /* __COMPILER_H */
