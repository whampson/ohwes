/* =============================================================================
 * Copyright (C) 2020-2023 Wes Hampson. All Rights Reserved.
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
 *         File: include/ohwes/io.h
 *      Created: December 13, 2020
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef _IO_H
#define _IO_H

#include <stdint.h>

/**
 * Write to this port to add a small delay to any I/O transaction.
 *
 * This port is typically used by the BIOS to report POST codes during boot.
 * By the time the OS is loaded, POST codes are pretty much useless. We can take
 * advantage of that fact and repurpose the port for I/O delay.
*/
#define IO_DELAY_PORT   0x80

/**
 * Reads a byte from an I/O port.
 */
static inline uint8_t inb(uint16_t port)
{
    uint8_t data;
    __asm__ volatile (
        "inb    %w1, %b0"
        : "=a"(data)
        : "d"(port)
    );

    return data;
}

/**
 * Reads a byte from an I/O port with a short delay before reading.
 */
static inline uint8_t inb_delay(uint16_t port)
{
    uint8_t data;
    __asm__ volatile (
        "                   \n\
        xorb    %b0, %b0    \n\
        outb    %b0, %w2    \n\
        inb     %w1, %b0    \n\
        "
        : "=a"(data)
        : "d"(port), "i"(IO_DELAY_PORT)
    );

    return data;
}


/**
 * Writes a byte to an I/O port.
 */
static inline void outb(uint16_t port, uint8_t data)
{
    __asm__ volatile (
        "outb   %b0, %w1"
        :
        : "a"(data), "d"(port)
    );
}

/**
 * Writes a byte to an I/O port with a short delay after writing.
 */
static inline void outb_delay(uint16_t port, uint8_t data)
{
    __asm__ volatile (
        "                   \n\
        outb    %b0, %w1    \n\
        xorb    %b0, %b0    \n\
        outb    %b0, %w2    \n\
        "
        :
        : "a"(data), "d"(port), "n"(IO_DELAY_PORT)
    );
}

#endif /* _IO_H */
