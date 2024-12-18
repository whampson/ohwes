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

#ifndef __ASSEMBLER__

#include <stddef.h>
#include <interrupt.h>

//
// Calling convention for System Calls.
//
#define __syscall __attribute__((regparm(0)))


//
// System Call invocation methods.
//

#define syscall0(nr)                                                            \
({                                                                              \
    int __sysret;                                                               \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(__sysret)                                                        \
        : "a"(nr)                                                               \
    );                                                                          \
    __sysret;                                                                   \
})

#define syscall1(nr,arg0)                                                       \
({                                                                              \
    int __sysret;                                                               \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(__sysret)                                                        \
        : "a"(nr), "b"(arg0)                                                    \
    );                                                                          \
    __sysret;                                                                   \
})

#define syscall2(nr,arg0,arg1)                                                  \
({                                                                              \
    int __sysret;                                                               \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(__sysret)                                                        \
        : "a"(nr), "b"(arg0), "c"(arg1)                                         \
    );                                                                          \
    __sysret;                                                                   \
})

#define syscall3(nr,arg0,arg1,arg2)                                             \
({                                                                              \
    int __sysret;                                                               \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(__sysret)                                                        \
        : "a"(nr), "b"(arg0), "c"(arg1), "d"(arg2)                              \
    );                                                                          \
    __sysret;                                                                   \
})

#define syscall4(nr,arg0,arg1,arg2,arg3)                                        \
({                                                                              \
    int __sysret;                                                               \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(__sysret)                                                        \
        : "a"(nr), "b"(arg0), "c"(arg1), "d"(arg2), "S"(arg3)                   \
    );                                                                          \
    __sysret;                                                                   \
})

#define syscall5(nr,arg0,arg1,arg2,arg3,arg4)                                   \
({                                                                              \
    int __sysret;                                                               \
    __asm__ volatile (                                                          \
        "int $0x80"                                                             \
        : "=a"(__sysret)                                                        \
        : "a"(nr), "b"(arg0), "c"(arg1), "d"(arg2), "S"(arg3), "D"(arg4)        \
    );                                                                          \
    __sysret;                                                                   \
})


//
// System Call number declarations.
//
#include <syscall_table.h>

//
// Define the kernel side of a System Call function.
//
#define SYSCALL_DEFINE(name, ...) \
    __syscall int sys_##name(__VA_ARGS__)

// #ifndef __KERNEL__

//
// System Call user mode function wrappers.
//

#define __SYSCALL_PROLOGUE      \
    int __ret
#define __SYSCALL_INVOKE(x)     \
    __ret = x
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

#define SYSCALL0_LINK(type,name)                                                \
type name(void)                                                                 \
{                                                                               \
    __SYSCALL_PROLOGUE;                                                         \
    __SYSCALL_INVOKE(syscall0(_SYS_##name));                                    \
    __SYSCALL_EPILOGUE;                                                         \
}

#define SYSCALL1_LINK(type,name,arg0_t,arg0)                                    \
type name(arg0_t arg0)                                                          \
{                                                                               \
    __SYSCALL_PROLOGUE;                                                         \
    __SYSCALL_INVOKE(syscall1(_SYS_##name,arg0));                               \
    __SYSCALL_EPILOGUE;                                                         \
}

#define SYSCALL1_LINK_VOID(name,arg0_t,arg0)                                    \
void name(arg0_t arg0)                                                          \
{                                                                               \
    __SYSCALL_PROLOGUE;                                                         \
    __SYSCALL_INVOKE(syscall1(_SYS_##name,arg0));                               \
    __SYSCALL_EPILOGUE_VOID;                                                    \
}


#define SYSCALL2_LINK(type,name,arg0_t,arg0,arg1_t,arg1)                        \
type name(arg0_t arg0, arg1_t arg1)                                             \
{                                                                               \
    __SYSCALL_PROLOGUE;                                                         \
    __SYSCALL_INVOKE(syscall2(_SYS_##name,arg0,arg1));                          \
    __SYSCALL_EPILOGUE;                                                         \
}

#define SYSCALL3_LINK(type,name,arg0_t,arg0,arg1_t,arg1,arg2_t,arg2)            \
type name(arg0_t arg0, arg1_t arg1, arg2_t arg2)                                \
{                                                                               \
    __SYSCALL_PROLOGUE;                                                         \
    __SYSCALL_INVOKE(syscall3(_SYS_##name,arg0,arg1,arg2));                     \
    __SYSCALL_EPILOGUE;                                                         \
}

#define SYSCALL4_LINK(type,name,arg0_t,arg0,arg1_t,arg1,arg2_t,arg2,arg3_t,arg3)\
type name(arg0_t arg0, arg1_t arg1, arg2_t arg2, arg3_t arg3)                   \
{                                                                               \
    __SYSCALL_PROLOGUE;                                                         \
    __SYSCALL_INVOKE(syscall4(_SYS_##name,arg0,arg1,arg2,arg3));                \
    __SYSCALL_EPILOGUE;                                                         \
}

#define SYSCALL5_LINK(type,name,arg0_t,arg0,arg1_t,arg1,arg2_t,arg2,arg3_t,arg3,arg4_t,arg4)\
type name(arg0_t arg0, arg1_t arg1, arg2_t arg2, arg3_t arg3, arg4_t arg4)      \
{                                                                               \
    __SYSCALL_PROLOGUE;                                                         \
    __SYSCALL_INVOKE(syscall5(_SYS_##name,arg0,arg1,arg2,arg3,arg4));           \
    __SYSCALL_EPILOGUE;                                                         \
}

// #endif  // __KERNEL__

#endif  // __ASSEMBLER__

#endif  // __SYSCALL_H
