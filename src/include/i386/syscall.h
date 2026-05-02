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
 *         File: include/syscall.h
 *      Created: March 24, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __SYSCALL_H
#define __SYSCALL_H

#ifndef __ASSEMBLER__

#include <stddef.h>
#include <i386/interrupt.h>
#include <i386/syscall_table.h>

#ifdef __KERNEL__

//
// Calling convention for System Call entry points.
//
#define __syscall __attribute__((regparm(0)))

//
// Declare a System Call entry point.
//
#define SYSCALL_ENTRY(name, ...) \
    __syscall int sys_##name(__VA_ARGS__)

#endif

//
// Generate System Call interrupt.
//

#define _syscall0_asm(nr)                                                       \
({                                                                              \
    int __sysret;                                                               \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(__sysret)                                                        \
        : "a"(nr)                                                               \
    );                                                                          \
    __sysret;                                                                   \
})

#define _syscall1_asm(nr,arg0)                                                  \
({                                                                              \
    int __sysret;                                                               \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(__sysret)                                                        \
        : "a"(nr), "b"(arg0)                                                    \
    );                                                                          \
    __sysret;                                                                   \
})

#define _syscall2_asm(nr,arg0,arg1)                                             \
({                                                                              \
    int __sysret;                                                               \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(__sysret)                                                        \
        : "a"(nr), "b"(arg0), "c"(arg1)                                         \
    );                                                                          \
    __sysret;                                                                   \
})

#define _syscall3_asm(nr,arg0,arg1,arg2)                                        \
({                                                                              \
    int __sysret;                                                               \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(__sysret)                                                        \
        : "a"(nr), "b"(arg0), "c"(arg1), "d"(arg2)                              \
    );                                                                          \
    __sysret;                                                                   \
})

#define _syscall4_asm(nr,arg0,arg1,arg2,arg3)                                   \
({                                                                              \
    int __sysret;                                                               \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(__sysret)                                                        \
        : "a"(nr), "b"(arg0), "c"(arg1), "d"(arg2), "S"(arg3)                   \
    );                                                                          \
    __sysret;                                                                   \
})

#define _syscall5_asm(nr,arg0,arg1,arg2,arg3,arg4)                              \
({                                                                              \
    int __sysret;                                                               \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(__sysret)                                                        \
        : "a"(nr), "b"(arg0), "c"(arg1), "d"(arg2), "S"(arg3), "D"(arg4)        \
    );                                                                          \
    __sysret;                                                                   \
})

// TODO: figure out how to get a 6th param via EBP in there...


//
// System Call link function wrapper.
//

#define __SYSCALL_PROLOGUE      \
    int __ret
#define __SYSCALL_INVOKE(fn)    \
    __ret = fn
#define __SYSCALL_EPILOGUE_VOID \
do {                            \
    if (__ret < 0) {            \
        errno = -__ret;         \
        __ret = -1;             \
    }                           \
} while (0)
#define __SYSCALL_EPILOGUE      \
    __SYSCALL_EPILOGUE_VOID;    \
    return __ret


//
// Define a C function that invokes a System Call interrupt.
//

#define LINK_SYSCALL0(type,name)                                                \
type name(void)                                                                 \
{                                                                               \
    __SYSCALL_PROLOGUE;                                                         \
    __SYSCALL_INVOKE(_syscall0_asm(_SYS_##name));                               \
    __SYSCALL_EPILOGUE;                                                         \
}

#define LINK_SYSCALL1(type,name,arg0_t,arg0)                                    \
type name(arg0_t arg0)                                                          \
{                                                                               \
    __SYSCALL_PROLOGUE;                                                         \
    __SYSCALL_INVOKE(_syscall1_asm(_SYS_##name,arg0));                          \
    __SYSCALL_EPILOGUE;                                                         \
}

#define LINK_SYSCALL2(type,name,arg0_t,arg0,arg1_t,arg1)                        \
type name(arg0_t arg0, arg1_t arg1)                                             \
{                                                                               \
    __SYSCALL_PROLOGUE;                                                         \
    __SYSCALL_INVOKE(_syscall2_asm(_SYS_##name,arg0,arg1));                     \
    __SYSCALL_EPILOGUE;                                                         \
}

#define LINK_SYSCALL3(type,name,arg0_t,arg0,arg1_t,arg1,arg2_t,arg2)            \
type name(arg0_t arg0, arg1_t arg1, arg2_t arg2)                                \
{                                                                               \
    __SYSCALL_PROLOGUE;                                                         \
    __SYSCALL_INVOKE(_syscall3_asm(_SYS_##name,arg0,arg1,arg2));                \
    __SYSCALL_EPILOGUE;                                                         \
}

#define LINK_SYSCALL4(type,name,arg0_t,arg0,arg1_t,arg1,arg2_t,arg2,arg3_t,arg3)\
type name(arg0_t arg0, arg1_t arg1, arg2_t arg2, arg3_t arg3)                   \
{                                                                               \
    __SYSCALL_PROLOGUE;                                                         \
    __SYSCALL_INVOKE(_syscall4_asm(_SYS_##name,arg0,arg1,arg2,arg3));           \
    __SYSCALL_EPILOGUE;                                                         \
}

#define LINK_SYSCALL5(type,name,arg0_t,arg0,arg1_t,arg1,arg2_t,arg2,arg3_t,arg3,arg4_t,arg4)\
type name(arg0_t arg0, arg1_t arg1, arg2_t arg2, arg3_t arg3, arg4_t arg4)      \
{                                                                               \
    __SYSCALL_PROLOGUE;                                                         \
    __SYSCALL_INVOKE(_syscall5_asm(_SYS_##name,arg0,arg1,arg2,arg3,arg4));      \
    __SYSCALL_EPILOGUE;                                                         \
}

#endif  // __ASSEMBLER__

#endif  // __SYSCALL_H
