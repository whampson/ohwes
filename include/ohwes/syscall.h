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

#define SYS_TEST0       0
#define SYS_TEST1       1
#define SYS_TEST2       2
#define SYS_TEST3       3
#define SYS_TEST4       4
#define SYS_TEST5       5
#define NUM_SYSCALL     6

#define __syscall_setup                                                     \
    int __ret;

#define __syscall0(fn)                                                      \
__asm__ volatile (                                                          \
    "int $0x80"                                                             \
    : "=a"(__ret)                                                           \
    : "a"(fn)                                                               \
)

#define __syscall1(fn,arg1)                                                 \
__asm__ volatile (                                                          \
    "int $0x80"                                                             \
    : "=a"(__ret)                                                           \
    : "a"(fn), "b"(arg1)                                                    \
)

#define __syscall2(fn,arg1,arg2)                                            \
__asm__ volatile (                                                          \
    "int $0x80"                                                             \
    : "=a"(__ret)                                                           \
    : "a"(fn), "b"(arg1), "c"(arg2)                                         \
)

#define __syscall3(fn,arg1,arg2,arg3)                                       \
__asm__ volatile (                                                          \
    "int $0x80"                                                             \
    : "=a"(__ret)                                                           \
    : "a"(fn), "b"(arg1), "c"(arg2), "d"(arg3)                              \
)

#define __syscall4(fn,arg1,arg2,arg3,arg4)                                  \
__asm__ volatile (                                                          \
    "int $0x80"                                                             \
    : "=a"(__ret)                                                           \
    : "a"(fn), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4)                   \
)

#define __syscall5(fn,arg1,arg2,arg3,arg4,arg5)                             \
__asm__ volatile (                                                          \
    "int $0x80"                                                             \
    : "=a"(__ret)                                                           \
    : "a"(fn), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4), "D"(arg5)        \
)

#define __syscall_ret                                                       \
do {                                                                        \
    if (__ret < 0) {                                                        \
        errno = -__ret;                                                     \
        return -1;                                                          \
    }                                                                       \
    return __ret;                                                           \
} while (0)

#endif /* __SYSCALL_H */
