/*============================================================================*
 * Copyright (C) 2020 Wes Hampson. All Rights Reserved.                       *
 *                                                                            *
 * This file is part of the Niobium Operating System.                         *
 * Niobium is free software; you may redistribute it and/or modify it under   *
 * the terms of the license agreement provided with this software.            *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: drivers/vga/vga.c                                                 *
 * Created: December 14, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#include <drivers/vga.h>
#include <nb/io.h>

void vga_init(void)
{
    /* ensure Color Text Mode ports are being used */
    outb(VGA_PORT_EXTL_MO_W, inb(VGA_PORT_EXTL_MO_R) | VGA_FLD_EXTL_MO_IOAS);
}

uint8_t vga_crtc_read(uint8_t reg)
{
    // From OSDever... TODO?
    // 1. Input the value of the Address Register and save it for step 6
    // 2. Output the index of the desired Data Register to the Address Register.
    // 3. Read the value of the Data Register and save it for later restoration upon termination, if needed.
    // 4. If writing, modify the value read in step 3, making sure to mask off bits not being modified.
    // 5. If writing, write the new value from step 4 to the Data register.
    // 6. Write the value of Address register saved in step 1 to the Address Register.
    
    /* TODO: delay? */

    outb(VGA_PORT_CRTC_ADDR, reg);
    return inb(VGA_PORT_CRTC_DATA);
}

void vga_crtc_write(uint8_t reg, uint8_t data)
{
    /* TODO: delay? */

    outb(VGA_PORT_CRTC_ADDR, reg);
    outb(VGA_PORT_CRTC_DATA, data);
}

uint8_t vga_attr_read(uint8_t reg)
{
    // From OSDever...
    // 1. Input a value from the Input Status #1 Register (normally port 3DAh) and discard it.
    // 2. Read the value of the Address/Data Register and save it for step 7.
    // 3. Output the index of the desired Data Register to the Address/Data Register
    // 4. Read the value of the Data Register and save it for later restoration upon termination, if needed.
    // 5. If writing, modify the value read in step 4, making sure to mask off bits not being modified.
    // 6. If writing, write the new value from step 5 to the Address/Data register.
    // 7. Write the value of Address register saved in step 1 to the Address/Data Register.
    // 8. If you wish to leave the register waiting for an index, input a value from the Input Status #1 Register (normally port 3DAh) and discard it.

    /* TODO: the above... */
    /* TODO: delay? */

    (void) inb(VGA_PORT_EXTL_IS1);
    outb(VGA_PORT_ATTR_ADDR, (reg & VGA_FLD_ATTR_ADDR_ADDR) | VGA_FLD_ATTR_ADDR_PAS);
    return inb(VGA_PORT_ATTR_DATA_R);
}

void vga_attr_write(uint8_t reg, uint8_t data)
{
    /* TODO: delay? */
    
    (void) inb(VGA_PORT_EXTL_IS1);
    outb(VGA_PORT_ATTR_ADDR, (reg & VGA_FLD_ATTR_ADDR_ADDR) | VGA_FLD_ATTR_ADDR_PAS);
    outb(VGA_PORT_ATTR_DATA_W, data);
}
