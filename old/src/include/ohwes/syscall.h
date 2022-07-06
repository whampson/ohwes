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
 *    File: include/ohwes/syscall.h                                           *
 * Created: December 21, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#ifndef __SYSCALL_H
#define __SYSCALL_H

/* System Call Numbers */
#define SYS_READ        0
#define SYS_WRITE       1

/**
 * System call setup routine for library functions.
 * Use this before invoking one of the syscallN() macros.
 */
#define syscall_setup                                                       \
    int __ret;

/**
 * Invokes a system call with zero parameters.
 */
#define syscall0(n)                                                         \
__asm__ volatile (                                                          \
    "int $0x80"                                                             \
    : "=a"(__ret)                                                           \
    : "a"(n)                                                                \
)

/**
 * Invokes a system call with one parameter.
 */
#define syscall1(n,arg1)                                                    \
__asm__ volatile (                                                          \
    "int $0x80"                                                             \
    : "=a"(__ret)                                                           \
    : "a"(n), "b"(arg1)                                                     \
)

/**
 * Invokes a system call with two parameters.
 */
#define syscall2(n,arg1,arg2)                                               \
__asm__ volatile (                                                          \
    "int $0x80"                                                             \
    : "=a"(__ret)                                                           \
    : "a"(n), "b"(arg1), "c"(arg2)                                          \
)

/**
 * Invokes a system call with three parameters.
 */
#define syscall3(n,arg1,arg2,arg3)                                          \
__asm__ volatile (                                                          \
    "int $0x80"                                                             \
    : "=a"(__ret)                                                           \
    : "a"(n), "b"(arg1), "c"(arg2), "d"(arg3)                               \
)

/**
 * Invokes a system call with four parameters.
 */
#define syscall4(n,arg1,arg2,arg3,arg4)                                     \
__asm__ volatile (                                                          \
    "int $0x80"                                                             \
    : "=a"(__ret)                                                           \
    : "a"(n), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4)                    \
)

/**
 * Invokes a system call with five parameters.
 */
#define syscall5(n,arg1,arg2,arg3,arg4,arg5)                                \
__asm__ volatile (                                                          \
    "int $0x80"                                                             \
    : "=a"(__ret)                                                           \
    : "a"(n), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4), "D"(arg5)         \
)

/**
 * Returns from the calling function after exiting a system call.
 * Sets errno then returns -1 if the system call returned a negative result,
 * otherwise returns the syscall return value.
 */
#define syscall_ret                                                         \
do {                                                                        \
    if (__ret < 0) {                                                        \
        errno = -__ret;                                                     \
        return -1;                                                          \
    }                                                                       \
    return __ret;                                                           \
} while (0)

#endif /* __SYSCALL_H */
