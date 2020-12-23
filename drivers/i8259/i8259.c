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
 *    File: drivers/i8259/i8259.c                                             *
 * Created: December 23, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#include <drivers/i8259.h>
#include <ohwes/io.h>

static uint16_t get_data_port(int pic_num)
{
    return (pic_num % 2) ? I8259_PORT_PIC1_DATA : I8259_PORT_PIC0_DATA;
}

static uint16_t get_cmd_port(int pic_num)
{
    return (pic_num % 2) ? I8259_PORT_PIC1_CMD : I8259_PORT_PIC0_CMD;
}

uint8_t i8259_data_read(int pic_num)
{
    return inb_p(get_data_port(pic_num));
}

void i8259_data_write(int pic_num, uint8_t data)
{
    outb_p(get_data_port(pic_num), data);
}

void i8259_cmd_write(int pic_num, uint8_t data)
{
    outb_p(get_cmd_port(pic_num), data);
}
