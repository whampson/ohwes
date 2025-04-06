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
 *         File: include/bitops.h
 *      Created: December 13, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __BITOPS_H
#define __BITOPS_H

/**
 * Set a bit in a bitstring.
 *
 * @param addr bitstring address
 * @param index index of the bit to set
 */
static inline void set_bit(volatile void *addr, unsigned int index)
{
    volatile char *bits = (volatile char *) addr;
    __asm__ volatile ("lock btsl %1, %0" : "+m"(*bits) : "Ir"(index));
}

/**
 * Clear a bit in a bitstring.
 *
 * @param addr bitstring address
 * @param index index of the bit to clear
 */
static inline void clear_bit(volatile void *addr, unsigned int index)
{
    volatile char *bits = (volatile char *) addr;
    __asm__ volatile ("lock btrl %1, %0" : "+m"(*bits) : "Ir"(index));
}

/**
 * Toggle a bit in a bitstring.
 *
 * @param addr bitstring address
 * @param index index of the bit to flip
 */
static inline void flip_bit(volatile void *addr, unsigned int index)
{
    volatile char *bits = (volatile char *) addr;
    __asm__ volatile ("lock btcl %1, %0" : "+m"(*bits) : "Ir"(index));
}

/**
 * Get the value of a bit in a bitstring.
 *
 * @param addr bitstring address
 * @param index index of the bit to test
 * @return the bit value
 */
static inline int test_bit(volatile void *addr, unsigned int index)
{
    volatile char *bits = (volatile char *) addr;

    // incredibly clever usage of SBB here to extract carry flag, taken from:
    //   https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html#OutputOperand
    // saves us a push and a pop :)

    int bit;
    __asm__ volatile (
        "                   \n\
        btl     %2, %1      \n\
        sbb     %0, %0      \n\
        "
        : "=r"(bit)
        : "m"(*bits), "Ir"(index)
    );

    return bit;
}

/**
 * Set a bit in a bitstring, then return the previous value of the set bit.
 *
 * @param addr bitstring address
 * @param index index of the bit to set
 * @return the previous value of the set bit
 */
static inline int test_and_set_bit(volatile void *addr, unsigned int index)
{
    volatile char *bits = (volatile char *) addr;

    int bit;
    __asm__ volatile (
        "                   \n\
   lock btsl    %2, %1      \n\
        sbb     %0, %0      \n\
        "
        : "=r"(bit), "=m"(*bits)
        : "Ir"(index)
    );

    return bit;
}

/**
 * Clear a bit in a bitstring, then return the previous value of the cleared
 * bit.
 *
 * @param addr bitstring address
 * @param index index of the bit to clear
 * @return the previous value of the cleared bit
 */
static inline int test_and_clear_bit(volatile void *addr, unsigned int index)
{
    volatile char *bits = (volatile char *) addr;

    int bit;
    __asm__ volatile (
        "                   \n\
   lock btrl    %2, %1      \n\
        sbb     %0, %0      \n\
        "
        : "=r"(bit), "=m"(*bits)
        : "Ir"(index)
    );

    return bit;
}

/**
 * Toggle a bit in a bitstring, then return the previous value of the toggled
 * bit.
 *
 * @param addr bitstring address
 * @param index index of the bit to flip
 * @return the previous value of the flipped bit
 */
static inline int test_and_flip_bit(volatile void *addr, unsigned int index)
{
    volatile char *bits = (volatile char *) addr;

    int bit;
    __asm__ volatile (
        "                   \n\
   lock btcl    %2, %1      \n\
        sbb     %0, %0      \n\
        "
        : "=r"(bit), "=m"(*bits)
        : "Ir"(index)
    );

    return bit;
}

/**
 * Find the first set bit in a bitstring.
 *
 * @param addr bitstring address
 * @param size number of bytes in bitstring; must be a multiple of 4 because
 *             underlying Intel BSF instruction operates in terms of DWORDs.
 * @return index of first set bit, or -1 if all bits are zero
 */
static inline int bit_scan_forward(volatile void *addr, unsigned int size)
{
    int dword_index = -1;
    int dword_count = size >> 2;
    int bit_index;

    __asm__ volatile (
        "                                   \n\
    1:                                      \n\
        incl    %%ecx                       \n\
        decl    %%edx                       \n\
        js      2f                          \n\
        bsfl    0(%%ebx, %%ecx, 4), %%eax   \n\
        jz      1b                          \n\
    2:                                      \n\
        "
        : "=a"(bit_index), "+c"(dword_index), "+d"(dword_count)
        : "b"(addr)
    );

    if (dword_count < 0) {
        return -1;
    }
    return (dword_index << 5) + bit_index;
}

#endif // __BITOPS_H
