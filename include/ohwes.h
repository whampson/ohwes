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
 *         File: include/ohwes.h
 *      Created: January 7, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __OHWES_H
#define __OHWES_H

#include <interrupt.h>
#include <x86.h>

#ifndef __ASSEMBLER__

#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <kernel.h>
//
// OS Version Info
//

#define OS_NAME                         "OH-WES"
#define OS_VERSION                      "0.1"
#define OS_MONIKER                      "Ronnie Raven"
#define OS_AUTHOR                       "whampson"

//
// Useful Kernel Macros
//

extern void timer_sleep(int millis);           // see timer.c
extern void pcspk_beep(int freq, int millis);  // see timer.c

#define beep(f,ms)                      pcspk_beep(f, ms)   // beep at frequency for millis (nonblocking)
#define sleep(ms)                       timer_sleep(ms)     // spin for millis (blocking)

#define die()                           ({ kprint("system halted"); for (;;); }) // spin forever, satisfies __noreturn
#define spin(cond)                      while (cond) { }    // spin while cond == true, TODO: THIS NEEDS TO HAVE A TIMEOUT!!

#define zeromem(p,n)                    memset(p, 0, n)

#define has_flag(x,f)                   (((x)&(f))==(f))
#define countof(x)                      (sizeof(x)/sizeof(x[0]))

#define align(x, n)                     (((x) + (n) - 1) & ~((n) - 1))
#define aligned(x,n)                    ((x) == align(x,n))

//
// CPU Privilege
//

enum pl {
    KERNEL_PL = 0,
    USER_PL = 3,
};

#define getpl()                         \
({                                      \
    struct segsel cs;                   \
    store_cs(cs);                       \
    cs.rpl;                             \
})

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

#define KB      (1 << 10)
#define MB      (1 << 20)
#define GB      (1 << 30)

static inline void set_bit(void *addr, uint32_t index)
{
    ((uint32_t *) addr)[index / 32] |= (1 << (index % 32));
}

static inline void clear_bit(void *addr, uint32_t index)
{
    ((uint32_t *) addr)[index / 32] &= ~(1 << (index % 32));
}

static inline void flip_bit(void *addr, uint32_t index)
{
    ((uint32_t *) addr)[index / 32] ^= (1 << (index % 32));
}

static inline bool test_bit(void *addr, uint32_t index)
{
    return ((uint32_t *) addr)[index / 32] & (1 << (index % 32));
}

#endif // __ASSEMBLER__

#endif // __OHWES_H
