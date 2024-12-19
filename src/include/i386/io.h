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
 *         File: include/io.h
 *      Created: January 22, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __IO_H
#define __IO_H

#include <stdint.h>

/**
 * CMOS Index Register (R/W)
 *
 * Bits 6:0 control CMOS RAM index.
 * Bit 7 is the Non-Maskable Interrupt disable bit.
 */
#define CMOS_INDEX_PORT         0x70

/**
 * CMOS Data Register (R/W)
 *
 *  Data read from or to be written to CMOS RAM.
 */
#define CMOS_DATA_PORT          0x71

/**
 * I/O Delay Port
 * Write to this port to add a small delay to any I/O transaction.
 *
 * This port is typically used by the BIOS to report POST codes during boot.
 * By the time the OS is loaded, POST codes are pretty much useless. We can take
 * advantage of that fact and repurpose the port for I/O delay.
*/
#define IO_DELAY_PORT           0x80

#define SYSCNTL_PORT_A          0x92    // read when NMI occurs
#define SYSCNTL_PORT_B          0x61    // read when NMI occurs

#define inb(port)                                                           \
({                                                                          \
    uint8_t _x;                                                             \
    __asm__ volatile (                                                      \
        "inb    %w1, %b0"                                                   \
        : "=a"(_x)                                                          \
        : "d"(port)                                                         \
    );                                                                      \
    _x;                                                                     \
})

#define inb_delay(port)                                                     \
({                                                                          \
    uint8_t _x;                                                             \
    __asm__ volatile (                                                      \
        "                                                                   \n\
        inb     %2                                                          \n\
        inb     %w1, %b0                                                    \n\
        "                                                                   \
        : "=a"(_x)                                                          \
        : "d"(port),                                                        \
          "i"(IO_DELAY_PORT)                                                \
    );                                                                      \
    _x;                                                                     \
})

#define outb(port, data)                                                    \
do {                                                                        \
    __asm__ volatile (                                                      \
        "outb   %b0, %w1"                                                   \
        :                                                                   \
        : "a"(data), "d"(port)                                              \
    );                                                                      \
} while (0)

#define outb_delay(port,data)                                               \
do {                                                                        \
    __asm__ volatile (                                                      \
        "                                                                   \n\
        outb    %b0, %w1                                                    \n\
        inb     %2                                                          \n\
        "                                                                   \
        :                                                                   \
        : "a"(data), "d"(port),                                             \
          "i"(IO_DELAY_PORT)                                                \
    );                                                                      \
} while (0)

#define cmos_read(addr)                                                     \
({                                                                          \
    outb_delay(CMOS_INDEX_PORT, addr);                                      \
    inb(CMOS_DATA_PORT);                                                    \
})

#define cmos_write(addr,data)                                               \
({                                                                          \
    outb_delay(CMOS_INDEX_PORT, addr);                                      \
    outb_delay(CMOS_DATA_PORT, data);                                       \
})

#define nmi_disable()                                                       \
do {                                                                        \
    cmos_write(CMOS_INDEX_PORT, cmos_read(CMOS_INDEX_PORT) | 0x80);         \
} while(0)

#define nmi_enable()                                                        \
do {                                                                        \
    cmos_write(CMOS_INDEX_PORT, cmos_read(CMOS_INDEX_PORT) & 0x7F);         \
} while(0)


#endif /* __IO_H */
