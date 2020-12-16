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
#include <nb/interrupt.h>
#include <nb/io.h>

void vga_init(void)
{
    /* Set IOAS bit to ensure the VGA interface expects the Color Text Mode
       ports where appropriate. */
    outb(VGA_PORT_EXTL_MO_W, inb(VGA_PORT_EXTL_MO_R) | VGA_FLD_EXTL_MO_IOAS);

    vga_disable_blink();
    vga_show_cursor();
}

/* TODO: color, external, sqeuencer, graphics registers */

uint8_t vga_crtc_read(uint8_t reg)
{
    int flags;
    uint8_t data;

    cli_save(flags);
    outb(VGA_PORT_CRTC_ADDR, reg);
    data = inb(VGA_PORT_CRTC_DATA);
    restore_flags(flags);

    return data;
}

void vga_crtc_write(uint8_t reg, uint8_t data)
{
    int flags;

    cli_save(flags);
    outb(VGA_PORT_CRTC_ADDR, reg);
    outb(VGA_PORT_CRTC_DATA, data);
    restore_flags(flags);
}

uint8_t vga_attr_read(uint8_t reg)
{
    int flags;
    uint8_t addr = reg & VGA_FLD_ATTR_ADDR_ADDR;
    uint8_t data;
    
    cli_save(flags);
    (void) inb(VGA_PORT_EXTL_IS1);
    outb(VGA_PORT_ATTR_ADDR, VGA_FLD_ATTR_ADDR_PAS | addr); /* keep PAS set */
    data = inb(VGA_PORT_ATTR_DATA_R);
    restore_flags(flags);

    return data;
}

void vga_attr_write(uint8_t reg, uint8_t data)
{
    int flags;
    uint8_t addr = reg & VGA_FLD_ATTR_ADDR_ADDR;
    
    cli_save(flags);
    (void) inb(VGA_PORT_EXTL_IS1);
    outb(VGA_PORT_ATTR_ADDR, VGA_FLD_ATTR_ADDR_PAS | addr); /* keep PAS set */
    outb(VGA_PORT_ATTR_DATA_W, data);
    restore_flags(flags);
}

void vga_disable_blink(void)
{
    uint8_t modectl;
    modectl = vga_attr_read(VGA_REG_ATTR_MODE);
    modectl &= ~VGA_FLD_ATTR_MODE_BLINK;
    vga_attr_write(VGA_REG_ATTR_MODE, modectl);
}

void vga_enable_blink(void)
{
    uint8_t modectl;
    modectl = vga_attr_read(VGA_REG_ATTR_MODE);
    modectl |= VGA_FLD_ATTR_MODE_BLINK;
    vga_attr_write(VGA_REG_ATTR_MODE, modectl);
}

void vga_hide_cursor(void)
{
    uint8_t css;
    css = vga_crtc_read(VGA_REG_CRTC_CSS);
    css |= VGA_FLD_CRTC_CSS_CD;
    vga_crtc_write(VGA_REG_CRTC_CSS, css);
}

void vga_show_cursor(void)
{
    uint8_t css;
    css = vga_crtc_read(VGA_REG_CRTC_CSS);
    css &= ~VGA_FLD_CRTC_CSS_CD;
    vga_crtc_write(VGA_REG_CRTC_CSS, css);
}

uint16_t vga_get_cursor_pos(void)
{
    uint8_t poshi, poslo;
    poshi = vga_crtc_read(VGA_REG_CRTC_CL_HI);
    poslo = vga_crtc_read(VGA_REG_CRTC_CL_LO);
    return (poshi << 8) | poslo;
}

void vga_set_cursor_pos(uint16_t pos)
{
    vga_crtc_write(VGA_REG_CRTC_CL_HI, pos >> 8);
    vga_crtc_write(VGA_REG_CRTC_CL_LO, pos & 0xFF);
}

uint16_t vga_get_cursor_shape(void)
{
    uint8_t shapehi, shapelo;
    shapelo = vga_crtc_read(VGA_REG_CRTC_CSS) & VGA_FLD_CRTC_CSS_CSS;
    shapehi = vga_crtc_read(VGA_REG_CRTC_CSE) & VGA_FLD_CRTC_CSE_CSE;
    return (shapehi << 8) | shapelo;
}

void vga_set_cursor_shape(uint8_t start, uint8_t end)
{
    vga_crtc_write(VGA_REG_CRTC_CSS, start & VGA_FLD_CRTC_CSS_CSS);
    vga_crtc_write(VGA_REG_CRTC_CSE, end   & VGA_FLD_CRTC_CSE_CSE);
}