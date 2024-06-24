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
 *         File: include/syscall.h
 *      Created: March 24, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __SYSCALL_H
#define __SYSCALL_H

#include <stddef.h>
#include <interrupt.h>

//
// syscall numbers
//
#define _sys_init                       0
#define _sys_exit                       1
#define _sys_read                       2
#define _sys_write                      3
#define _sys_open                       4
#define _sys_close                      5
#define _sys_ioctl                      6
#define NUM_SYSCALLS                    7

#ifndef __ASSEMBLER__

//
// syscall C function prototypes
// symbol names must match the above _sys_* defines
//
extern int exit(int);
extern int read(int, void *, size_t);
extern int write(int, const void *, size_t);
extern int open(const char *, int);
extern int close(int);
extern int ioctl(int, unsigned int, void*);

//
// syscall calling convention
//
#define __syscall __attribute__((regparm(0)))


#define _SYSCALL_PROLOGUE                                                       \
    int _retval                                                                 \

#define _SYSCALL_EPILOGUE                                                       \
    if (_retval < 0) {                                                          \
        errno = -_retval;                                                       \
        return -1;                                                              \
    }                                                                           \
    return _retval                                                              \

#define _syscall1(fn,p0_t,p0)                                                   \
int fn(p0_t p0)                                                                 \
{                                                                               \
    _SYSCALL_PROLOGUE;                                                          \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(_retval)                                                         \
        : "a"(_sys_##fn), "b"(p0)                                               \
    );                                                                          \
    _SYSCALL_EPILOGUE;                                                          \
}

#define _syscall2(fn,p0_t,p0,p1_t,p1)                                           \
int fn(p0_t p0, p1_t p1)                                                        \
{                                                                               \
    _SYSCALL_PROLOGUE;                                                          \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(_retval)                                                         \
        : "a"(_sys_##fn), "b"(p0), "c"(p1)                                      \
    );                                                                          \
    _SYSCALL_EPILOGUE;                                                          \
}

#define _syscall3(fn,p0_t,p0,p1_t,p1,p2_t,p2)                                   \
int fn(p0_t p0, p1_t p1, p2_t p2)                                               \
{                                                                               \
    _SYSCALL_PROLOGUE;                                                          \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(_retval)                                                         \
        : "a"(_sys_##fn), "b"(p0), "c"(p1), "d"(p2)                             \
    );                                                                          \
    _SYSCALL_EPILOGUE;                                                          \
}

#define _syscall4(fn,p0_t,p0,p1_t,p1,p2_t,p2,p3_t,p3)                           \
int fn(p0_t p0, p1_t p1, p2_t p2, p3_t p3)                                      \
{                                                                               \
    _SYSCALL_PROLOGUE;                                                          \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(_retval)                                                         \
        : "a"(_sys_##fn), "b"(p0), "c"(p1), "d"(p2), "S"(p3)                    \
    );                                                                          \
    _SYSCALL_EPILOGUE;                                                          \
}

#define _syscall5(fn,p0_t,p0,p1_t,p1,p2_t,p2,p3_t,p3,p4_t,p4)                   \
int fn(p0_t p0, p1_t p1, p2_t p2, p3_t p3, p4_t p4)                             \
{                                                                               \
    _SYSCALL_PROLOGUE;                                                          \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(_retval)                                                         \
        : "a"(_sys_##fn), "b"(p0), "c"(p1), "d"(p2), "S"(p3), "D"(p4)           \
    );                                                                          \
    _SYSCALL_EPILOGUE;                                                          \
}

#endif  // __ASSEMBLER__

#endif  // __SYSCALL_H
