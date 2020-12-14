/*============================================================================*
 * Copyright (C) 2020 Wes Hampson. All Rights Reserved.                       *
 *                                                                            *
 * This file is part of the Niobium Operating System.                         *
 * Niobium is free software; you may redistribute it and/or modify it under   *
 * the terms of the license agreement provided with this software.            *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: include/string.h                                                  *
 * Created: December 13, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *                                                                            *
 * Implementation of string.h from the C11 Standard Library.                  *
 *============================================================================*/

/* Completion Status: INCOMPLETE */

#ifndef __STRING_H
#define __STRING_H

#include <stdint.h>

#ifndef __SIZE_T_DEFINED
#define __SIZE_T_DEFINED
typedef uint32_t size_t;
#endif

#ifndef __NULL_DEFINED
#define __NULL_DEFINED
#define NULL ((void *) 0)
#endif

static inline void * memset(void *dest, int c, size_t n)
{
    // TODO: TEST!!!
    unsigned char ch;
    ch = (unsigned char) c;

    __asm__ volatile (
        "                               \n\
    .memset_top%=:                      \n\
        testl   %%ecx, %%ecx            \n\
        jz      .memset_done%=          \n\
        testl   $3, %%edi               \n\
        jz      .memset_aligned%=       \n\
        movb    %%al, (%%edi)           \n\
        addl    $1, %%edi               \n\
        subl    $1, %%ecx               \n\
    .memset_aligned%=:                  \n\
        movw    %%ds, %%dx              \n\
        movw    %%dx, %%es              \n\
        movl    %%ecx, %%edx            \n\
        shrl    $2, %%ecx               \n\
        andl    $3, %%edx               \n\
        cld                             \n\
        rep     stosl                   \n\
    .memset_bottom%=:                   \n\
        testl   %%edx, %%edx            \n\
        jz      .memset_done%=          \n\
        movb    %%al, (%%edi)           \n\
        addl    $1, %%edi               \n\
        subl    $1, %%edx               \n\
    .memset_done%=:                     \n\
        "
        :
        : "D"(dest), "a"(ch << 24 | ch << 16 | ch << 8 | ch), "c"(n)
        : "edx", "memory", "cc"
    );

    return dest;
}

static inline void * memmove(void *dest, const void *src, size_t n)
{
    // TODO: TEST!!!
    __asm__ volatile (
        "                               \n\
        movw    %%ds, %%dx              \n\
        movw    %%dx, %%es              \n\
        cld                             \n\
        cmpl    %%edi, %%esi            \n\
        jae     .memmove_start%=        \n\
        leal    -1(%%esi, %%ecx), %%esi \n\
        leal    -1(%%edi, %%ecx), %%edi \n\
        std                             \n\
    .memmove_start%=:                   \n\
        rep     movsb                   \n\
        "
        :
        : "D"(dest), "S"(src), "c"(n)
        : "edx", "cc", "memory"
    );

    return dest;
}

#endif /* __STRING_H */
