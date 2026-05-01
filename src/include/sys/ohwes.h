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
 *         File: src/include/sys/ohwes.h
 *      Created: April 29, 2026
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __SYS_OHWES_H
#define __SYS_OHWES_H

#include <string.h>

#define _STRINGIFY(a)       # a
#define STRINGIFY(a)        _STRINGIFY(a)
#define CONCAT(a,b)         a ## b
#define PLURALIZE(n,a)      (((n)==1)?a:a "s")
#define PLURALIZE2(n,a,b)   (((n)==1)?a:b)
#define A_OR_B(n,a,b)       ((n)?a:b)

#define align(x, n)         (((uintptr_t)(x)+(n)-1)&(~((n)-1)))
#define isaligned(x,n)      ((uintptr_t)(x) == align(x,n))
#define ispow2(x)           ((x)==1 || ((x)&((x)-1))==0)
#define countof(x)          (sizeof(x)/sizeof((x)[0]))
#define zeromem(p,n)        memset(p, 0, n)

#define div_round(n,d)      (((n)<0) == ((d)<0) ? (((n)+(d)/2)/(d)) : (((n)-(d)/2)/(d)))
#define div_ceil(n,d)       (((n)+(d)-1)/(d))

#define min(a,b) ({ \
    __typeof__(a) __a = (a); \
    __typeof__(a) __b = (b); \
    __a < __b ? __a : __b; })

#define max(a,b) ({ \
    __typeof__(a) __a = (a); \
    __typeof__(a) __b = (b); \
    __a > __b ? __a : __b; })

#define swap(a,b) ({ \
    (a) ^= (b); \
    (b) ^= (a); \
    (a) ^= (b); \
})

#define VPRINT_CALL(fmt, fn) \
({ \
    va_list __args; \
    int __ret; \
    va_start(__args, fmt); \
    __ret = fn(fmt, __args); \
    va_end(__args); \
    __ret; \
})

#define VPRINT_CALL_V(fmt, fn, ...) \
({ \
    va_list __args; \
    int __ret; \
    va_start(__args, fmt); \
    __ret = fn(__VA_ARGS__, fmt, __args); \
    va_end(__args); \
    __ret; \
})

#endif // __SYS_OHWES_H
