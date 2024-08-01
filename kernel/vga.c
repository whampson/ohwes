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
 *         File: kernel/vga.c
 *      Created: December 14, 2020
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <interrupt.h>
#include <io.h>
#include <vga.h>
#include <x86.h>

uint8_t vga_crtc_read(uint8_t reg)
{
    int flags;
    uint8_t data;

    cli_save(flags);
    outb(VGA_CRTC_PORT_ADDR, reg);
    data = inb(VGA_CRTC_PORT_DATA);
    restore_flags(flags);

    return data;
}

void vga_crtc_write(uint8_t reg, uint8_t data)
{
    int flags;

    cli_save(flags);
    outb(VGA_CRTC_PORT_ADDR, reg);
    outb(VGA_CRTC_PORT_DATA, data);
    restore_flags(flags);
}

uint8_t vga_grfx_read(uint8_t reg)
{
    int flags;
    uint8_t data;

    cli_save(flags);
    outb(VGA_GRFX_PORT_ADDR, reg);
    data = inb(VGA_GRFX_PORT_DATA);
    restore_flags(flags);

    return data;
}

void vga_grfx_write(uint8_t reg, uint8_t data)
{
    int flags;

    cli_save(flags);
    outb(VGA_GRFX_PORT_ADDR, reg);
    outb(VGA_GRFX_PORT_DATA, data);
    restore_flags(flags);
}

uint8_t vga_seqr_read(uint8_t reg)
{
    int flags;
    uint8_t data;

    cli_save(flags);
    outb(VGA_SEQR_PORT_ADDR, reg);
    data = inb(VGA_SEQR_PORT_DATA);
    restore_flags(flags);

    return data;
}

void vga_seqr_write(uint8_t reg, uint8_t data)
{
    int flags;

    cli_save(flags);
    outb(VGA_SEQR_PORT_ADDR, reg);
    outb(VGA_SEQR_PORT_DATA, data);
    restore_flags(flags);
}

uint8_t vga_attr_read(uint8_t reg)
{
    int flags;
    uint8_t addr = reg & VGA_ATTR_FLD_ADDR_ADDR;
    uint8_t data;

    cli_save(flags);
    (void) inb(VGA_EXTL_PORT_IS1);
    outb(VGA_ATTR_PORT_ADDR, VGA_ATTR_FLD_ADDR_PAS | addr); /* keep PAS set */
    data = inb(VGA_ATTR_PORT_DATA_R);
    restore_flags(flags);

    return data;
}

void vga_attr_write(uint8_t reg, uint8_t data)
{
    int flags;
    uint8_t addr = reg & VGA_ATTR_FLD_ADDR_ADDR;

    cli_save(flags);
    (void) inb(VGA_EXTL_PORT_IS1);
    outb(VGA_ATTR_PORT_ADDR, VGA_ATTR_FLD_ADDR_PAS | addr); /* keep PAS set */
    outb(VGA_ATTR_PORT_DATA_W, data);
    restore_flags(flags);
}

uint8_t vga_extl_read(uint16_t port)
{
    return inb(port);
}

void vga_extl_write(uint16_t port, uint8_t data)
{
    outb(port, data);
}
