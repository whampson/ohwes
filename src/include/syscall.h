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

// user-callable kernel routines
extern void exit(int status);
extern int read(int fd, char *buf, size_t len);
extern int write(int fd, const char *buf, size_t len);

// syscall numbers
#define _sys_exit                       0
#define _sys_read                       1
#define _sys_write                      2
#define _sys_open                       3
#define _sys_close                      4
#define _sys_ioctl                      5
#define NUM_SYSCALLS                    6

#define _syscall_epilogue               \
({                                      \
    if (_retval < 0) {                  \
        errno = -_retval;               \
        return -1;                      \
    }                                   \
})

#define _syscall1(type,name,atype,a)                                            \
type name(atype a)                                                              \
{                                                                               \
    int _retval;                                                                \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(_retval)                                                         \
        : "a"(_sys_##name), "b"(a)                                              \
    );                                                                          \
    _syscall_epilogue;                                                          \
    return _retval;                                                             \
}

#define _syscall1_void(name,atype,a)                                            \
void name(atype a)                                                              \
{                                                                               \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        :                                                                       \
        : "a"(_sys_##name), "b"(a)                                              \
    );                                                                          \
}

#define _syscall3(type,name,atype,a,btype,b,ctype,c)                            \
type name(atype a, btype b, ctype c)                                            \
{                                                                               \
    int _retval;                                                                \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(_retval)                                                         \
        : "a"(_sys_##name), "b"(a), "c"(b), "d"(c)                              \
    );                                                                          \
    _syscall_epilogue;                                                          \
    return _retval;                                                             \
}



#endif //__SYSCALL_H
