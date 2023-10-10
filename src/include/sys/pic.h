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
 *         File: kernel/include/pic.h
 *      Created: December 22, 2020
 *       Author: Wes Hampson
 *
 * Intel 8259A Programmable Interrupt Controller driver.
 * =============================================================================
 */

#ifndef __PIC_H
#define __PIC_H

#include <stdint.h>
#include <sys/io.h>

/* I/O Ports */
#define I8259_PORT_PIC0_CMD     0x20        /* Master PIC Command Port */
#define I8259_PORT_PIC0_DATA    0x21        /* Master PIC Data Port */
#define I8259_PORT_PIC1_CMD     0xA0        /* Slave PIC Command Port */
#define I8259_PORT_PIC1_DATA    0xA1        /* Slave PIC Data Port */

static inline uint8_t i8259_data_read(int pic_num)
{
    uint16_t port = (pic_num % 2) ? I8259_PORT_PIC1_DATA : I8259_PORT_PIC0_DATA;
    return inb_delay(port);
}

static inline void i8259_data_write(int pic_num, uint8_t data)
{
    uint16_t port = (pic_num % 2) ? I8259_PORT_PIC1_DATA : I8259_PORT_PIC0_DATA;
    outb_delay(port, data);
}

static inline void i8259_cmd_write(int pic_num, uint8_t data)
{
    uint16_t port = (pic_num % 2) ? I8259_PORT_PIC1_CMD : I8259_PORT_PIC0_CMD;
    outb_delay(port, data);
}

#endif /* __PIC_H */
