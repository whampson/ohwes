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
 *         File: src/include/kernel/ohwes.h
 *      Created: January 7, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __OHWES_H
#define __OHWES_H

// TODO: try to remove this header

#ifndef __ASSEMBLER__

#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <kernel/kernel.h>

//
// Useful Kernel Macros
//

#define spin(cond)                      while (cond) { }    // spin while cond == true, TODO: THIS NEEDS TO HAVE A TIMEOUT!!

#define has_flag(x,f)                   (((x)&(f))==(f))
#define countof(x)                      (sizeof(x)/sizeof(x[0]))

#define align(x, n)                     (((x) + (n) - 1) & ~((n) - 1))
#define aligned(x,n)                    ((x) == align(x,n))

#define ispow2(x)                       ((x) == 1 || ((x) & ((x) - 1)) == 0)


//
// Strings
//

#define STRINGIFY(x)                    # x
#define STRINGIFY_LITERAL(x)            STRINGIFY(x)
#define CONCAT(a,b)                     a ## b

#define HASNO(cond)                     ((cond)?"has":"no")
#define YN(cond)                        ((cond)?"yes":"no")
#define ONOFF(cond)                     ((cond)?"on":"off")
#define PLURAL(n,a)                     (((n)==1)?a:a "s")
#define PLURAL2(n,a,b)                  (((n)==1)?a:b)


//
// Math
//

#define swap(a,b)                       \
do {                                    \
    (a) ^= (b);                         \
    (b) ^= (a);                         \
    (a) ^= (b);                         \
} while(0)

#define div_round(n,d)                  (((n)<0)==((d)<0)?(((n)+(d)/2)/(d)):(((n)-(d)/2)/(d)))
#define div_ceil(n,d)                   (((n)+(d)-1)/(d))

#define min(a,b) ({ \
    __typeof__(a) __a = (a); \
    __typeof__(a) __b = (b); \
    __a < __b ? __a : __b; })

#define max(a,b) ({ \
    __typeof__(a) __a = (a); \
    __typeof__(a) __b = (b); \
    __a > __b ? __a : __b; })

#endif // __ASSEMBLER__

#endif // __OHWES_H
