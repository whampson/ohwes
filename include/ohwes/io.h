/*============================================================================*
 * Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                  *
 *                                                                            *
 * This file is part of the OHWES Operating System.                           *
 * OHWES is free software; you may redistribute it and/or modify it under the *
 * terms of the license agreement provided with this software.                *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: include/ohwes/io.h                                                *
 * Created: December 13, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#ifndef __IO_H
#define __IO_H

#include <ohwes/types.h>

ssize_t read(int fd, void *buf, size_t n);
ssize_t write(int fd, const void *buf, size_t n);

/**
 * Reads a byte from an I/O port.
 *
 * @param port the port to read from
 * @return the byte read
 */
static inline uint8_t inb(uint16_t port)
{
    uint8_t data;
    __asm__ volatile (
        "inb %w1, %b0"
        : "=a"(data)
        : "d"(port)
        :
    );
    return data;
}

/**
 * Writes a byte to an I/O port.
 *
 * @param port the port to write to
 * @param data the byte to write
 */
static inline void outb(uint16_t port, uint8_t data)
{
    __asm__ volatile (
        "outb %b0, %w1"
        :
        : "a"(data), "d"(port)
        :
    );
}

#endif /* __IO_H */
