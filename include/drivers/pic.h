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
 *    File: include/drivers/pic.h                                             *
 * Created: December 22, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *                                                                            *
 * Intel 8259A Programmable Interrupt Controller driver.                      *
 *============================================================================*/

#ifndef __PIC_H
#define __PIC_H

#include <stdint.h>

/* I/O Ports */
#define I8259_PORT_PIC0_CMD     0x20        /* Master PIC Command Port */
#define I8259_PORT_PIC0_DATA    0x21        /* Master PIC Data Port */
#define I8259_PORT_PIC1_CMD     0xA0        /* Slave PIC Command Port */
#define I8259_PORT_PIC1_DATA    0xA1        /* Slave PIC Data Port */

uint8_t i8259_data_read(int pic_num);
void i8259_data_write(int pic_num, uint8_t data);
void i8259_cmd_write(int pic_num, uint8_t data);

#endif /* __PIC_H */
