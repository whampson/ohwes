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

#include <vga.h>
#include <x86.h>
#include <io.h>

void init_vga(void)
{
    //
    // The BIOS should've dropped us off in VGA mode 3, so we can assume text
    // mode, 80x25, 9x16 chars, 16 colors;
    // but let's do a few sanity checks here anyways.
    //

    // ensure the I/O Address Select bit is set so we are using non-monochrome
    // CRTC ports 0x3D4 and 0x3D5
    uint8_t misc_out = vga_extl_read(VGA_EXTL_PORT_MO_R);
    misc_out |= VGA_EXTL_FLD_MO_IOAS;
    vga_extl_write(VGA_EXTL_PORT_MO_W, misc_out);
    // TODO: readback & check?

    // ensure 0xB8000 is selected as the frame buffer address
    uint8_t grfx_misc = vga_grfx_read(VGA_GRFX_REG_MISC);
    uint8_t mmap_sel = VGA_GRFX_ENUM_MISC_MMAP_32K_HI;  // 0xB8000-0xBFFFF
    grfx_misc |= (mmap_sel << 2);   // bits [3:2] are the memory map select
    vga_grfx_write(VGA_GRFX_REG_MISC, grfx_misc);
    // TODO: readback & check?

    // disable blink by default
    vga_disable_blink();

    // make sure the cursor is visible
    vga_show_cursor();
}

void vga_disable_blink(void)
{
    uint8_t modectl;
    modectl = vga_attr_read(VGA_ATTR_REG_MODE);
    modectl &= ~VGA_ATTR_FLD_MODE_BLINK;        // clear blink bit
    vga_attr_write(VGA_ATTR_REG_MODE, modectl);
}

void vga_enable_blink(void)
{
    uint8_t modectl;
    modectl = vga_attr_read(VGA_ATTR_REG_MODE);
    modectl |= VGA_ATTR_FLD_MODE_BLINK;         // set blink bit
    vga_attr_write(VGA_ATTR_REG_MODE, modectl);
}

void vga_hide_cursor(void)
{
    uint8_t css;
    css = vga_crtc_read(VGA_CRTC_REG_CSS);  // CSS = cursor scan line
    css |= VGA_CRTC_FLD_CSS_CD_MASK;        // CD = cursor disable
    vga_crtc_write(VGA_CRTC_REG_CSS, css);
}

void vga_show_cursor(void)
{
    uint8_t css;
    css = vga_crtc_read(VGA_CRTC_REG_CSS);
    css &= ~VGA_CRTC_FLD_CSS_CD_MASK;
    vga_crtc_write(VGA_CRTC_REG_CSS, css);
}

uint16_t vga_get_cursor_pos(void)
{
    uint8_t poshi, poslo;
    poshi = vga_crtc_read(VGA_CRTC_REG_CL_HI);  // CL = cursor location
    poslo = vga_crtc_read(VGA_CRTC_REG_CL_LO);
    return (poshi << 8) | poslo;
}

void vga_set_cursor_pos(uint16_t pos)
{
    vga_crtc_write(VGA_CRTC_REG_CL_HI, pos >> 8);
    vga_crtc_write(VGA_CRTC_REG_CL_LO, pos & 0xFF);
}

uint16_t vga_get_cursor_shape(void)
{
    uint8_t shapehi, shapelo;
    shapelo = vga_crtc_read(VGA_CRTC_REG_CSS) & VGA_CRTC_FLD_CSS_CSS_MASK;
    shapehi = vga_crtc_read(VGA_CRTC_REG_CSE) & VGA_CRTC_FLD_CSE_CSE_MASK;
    return (shapehi << 8) | shapelo;
}

void vga_set_cursor_shape(uint8_t start, uint8_t end)
{
    vga_crtc_write(VGA_CRTC_REG_CSS, start & VGA_CRTC_FLD_CSS_CSS_MASK);
    vga_crtc_write(VGA_CRTC_REG_CSE, end   & VGA_CRTC_FLD_CSE_CSE_MASK);
}

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
