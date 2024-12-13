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
 *         File: include/bitops.h
 *      Created: December 13, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __BITOPS_H
#define __BITOPS_H

#include <stdbool.h>

static inline void set_bit(volatile void *addr, int index)
{
    volatile char *bits = (volatile char *) addr;
    __asm__ volatile ("btsl %1, %0" : "=m"(*bits) : "Ir"(index));
}

static inline void clear_bit(volatile void *addr, int index)
{
    volatile char *bits = (volatile char *) addr;
    __asm__ volatile ("btrl %1, %0" : "=m"(*bits) : "Ir"(index));
}

static inline void flip_bit(volatile void *addr, int index)
{
    volatile char *bits = (volatile char *) addr;
    __asm__ volatile ("btcl %1, %0" : "=m"(*bits) : "Ir"(index));
}

static inline bool test_bit(volatile void *addr, int index)
{
    volatile char *bits = (volatile char *) addr;

    int bit;
    __asm__ volatile ( // incredibly clever SBB usage here to extract carry flag
        "                   \n\
        btl     %2, %1      \n\
        sbb     %0, %0      \n\
        "
        : "=r"(bit)
        : "m"(*bits), "Ir"(index)
    );

    return bit;
}

static inline bool test_and_set_bit(volatile void *addr, int index)
{
    volatile char *bits = (volatile char *) addr;

    int bit;
    __asm__ volatile (
        "                   \n\
        btsl    %2, %1      \n\
        sbb     %0, %0      \n\
        "
        : "=r"(bit), "=m"(*bits)
        : "Ir"(index)
    );

    return bit;
}

static inline bool test_and_clear_bit(volatile void *addr, int index)
{
    volatile char *bits = (volatile char *) addr;

    int bit;
    __asm__ volatile (
        "                   \n\
        btrl    %2, %1      \n\
        sbb     %0, %0      \n\
        "
        : "=r"(bit), "=m"(*bits)
        : "Ir"(index)
    );

    return bit;
}

static inline bool test_and_flip_bit(volatile void *addr, int index)
{
    volatile char *bits = (volatile char *) addr;

    int bit;
    __asm__ volatile (
        "                   \n\
        btcl    %2, %1      \n\
        sbb     %0, %0      \n\
        "
        : "=r"(bit), "=m"(*bits)
        : "Ir"(index)
    );

    return bit;
}

static inline int bit_scan_forward(char *bits)
{
    int index = -1;
    __asm__ volatile ("bsfl %1, %0" : "=r"(index) : "m"(*bits));
    return index;
}

static inline int bit_scan_reverse(char *bits)
{
    int index = -1;
    __asm__ volatile ("bsrl %1, %0" : "=r"(index) : "m"(*bits));
    return index;
}

#endif // __BITOPS_H
