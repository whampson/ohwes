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
#define _sys_exit                       0
#define _sys_read                       1
#define _sys_write                      2
#define _sys_open                       3
#define _sys_close                      4
#define _sys_ioctl                      5
#define NUM_SYSCALLS                    6

#ifndef __ASSEMBLER__

//
// syscall C functions
// symbol names must match the above _sys_* defines
//
extern int exit(int);
extern int read(int, void *, size_t);
extern int write(int, const void *, size_t);
extern int open(const char *, int);
extern int close(int);
extern int ioctl(int, unsigned int, void*);

#define _SYSCALL_PROLOGUE                                                       \
    int _retval                                                                 \

#define _SYSCALL_EPILOGUE                                                       \
    /* set errno */                                                             \
    if (_retval < 0) {                                                          \
        errno = -_retval;                                                       \
        return -1;                                                              \
    }                                                                           \
    return _retval                                                              \

#define SYSCALL1(name,atype,a)                                                  \
int name(atype a)                                                               \
{                                                                               \
    _SYSCALL_PROLOGUE;                                                          \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(_retval)                                                         \
        : "a"(_sys_##name), "b"(a)                                              \
    );                                                                          \
    _SYSCALL_EPILOGUE;                                                          \
}

#define SYSCALL2(name,atype,a,btype,b)                                          \
int name(atype a, btype b)                                                      \
{                                                                               \
    _SYSCALL_PROLOGUE;                                                          \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(_retval)                                                         \
        : "a"(_sys_##name), "b"(a), "c"(b)                                      \
    );                                                                          \
    _SYSCALL_EPILOGUE;                                                          \
}

#define SYSCALL3(name,atype,a,btype,b,ctype,c)                                  \
int name(atype a, btype b, ctype c)                                             \
{                                                                               \
    _SYSCALL_PROLOGUE;                                                          \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(_retval)                                                         \
        : "a"(_sys_##name), "b"(a), "c"(b), "d"(c)                              \
    );                                                                          \
    _SYSCALL_EPILOGUE;                                                          \
}

#define SYSCALL4(name,atype,a,btype,b,ctype,c,dtype,d)                          \
int name(atype a, btype b, ctype c, dtype d)                                    \
{                                                                               \
    _SYSCALL_PROLOGUE;                                                          \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(_retval)                                                         \
        : "a"(_sys_##name), "b"(a), "c"(b), "d"(c), "S"(d)                      \
    );                                                                          \
    _SYSCALL_EPILOGUE;                                                          \
}

#define SYSCALL5(name,atype,a,btype,b,ctype,c,dtype,d,etype,e)                  \
int name(atype a, btype b, ctype c, dtype d, etype e)                           \
{                                                                               \
    _SYSCALL_PROLOGUE;                                                          \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(_retval)                                                         \
        : "a"(_sys_##name), "b"(a), "c"(b), "d"(c), "S"(d), "D"(e)              \
    );                                                                          \
    _SYSCALL_EPILOGUE;                                                          \
}

#ifdef __KERNEL__
#define __syscall __attribute__((regparm(0)))

#define _KERNEL_SYSCALL_PROLOGUE                                                \
    int _retval                                                                 \

#define _KERNEL_SYSCALL_EPILOGUE                                                \
    /* set errno */                                                             \
    if (_retval < 0) {                                                          \
        errno = -_retval;                                                       \
        return -1;                                                              \
    }                                                                           \
    return _retval                                                              \

#define KERNEL_SYSCALL1(name,atype,a)                                           \
extern __syscall int k##name(atype a);                                          \
int name(atype a)                                                               \
{                                                                               \
    _KERNEL_SYSCALL_PROLOGUE;                                                   \
    _retval = k##name(a);                                                       \
    _KERNEL_SYSCALL_EPILOGUE;                                                   \
}

#define KERNEL_SYSCALL2(name,atype,a,btype,b)                                   \
extern __syscall int k##name(atype a, btype b);                                 \
int name(atype a, btype b)                                                      \
{                                                                               \
    _KERNEL_SYSCALL_PROLOGUE;                                                   \
    _retval = k##name(a, b);                                                    \
    _KERNEL_SYSCALL_EPILOGUE;                                                   \
}

#define KERNEL_SYSCALL3(name,atype,a,btype,b,ctype,c)                           \
extern __syscall int k##name(atype a, btype b, ctype c);                        \
int name(atype a, btype b, ctype c)                                             \
{                                                                               \
    _KERNEL_SYSCALL_PROLOGUE;                                                   \
    _retval = k##name(a, b, c);                                                 \
    _KERNEL_SYSCALL_EPILOGUE;                                                   \
}

#define KERNEL_SYSCALL4(name,atype,a,btype,b,ctype,c,dtype,d)                   \
extern __syscall int k##name(atype a, btype b, ctype c, dtype d);               \
int name(atype a, btype b, ctype c, dtype d)                                    \
{                                                                               \
    _KERNEL_SYSCALL_PROLOGUE;                                                   \
    _retval = k##name(a, b, c, d);                                              \
    _KERNEL_SYSCALL_EPILOGUE;                                                   \
}

#define KERNEL_SYSCALL5(name,atype,a,btype,b,ctype,c,dtype,d,etype,e)           \
extern __syscall int k##name(atype a, btype b, ctype c, dtype d, etype e);      \
int name(atype a, btype b, ctype c, dtype d, etype e)                           \
{                                                                               \
    _KERNEL_SYSCALL_PROLOGUE;                                                   \
    _retval = k##name(a, b, c, d, e);                                           \
    _KERNEL_SYSCALL_EPILOGUE;                                                   \
}

#endif  // __KERNEL__

#endif  // __ASSEMBLER__

#endif  // __SYSCALL_H
