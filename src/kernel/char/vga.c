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
 *         File: kernel/char/vga.c
 *      Created: December 14, 2020
 *       Author: Wes Hampson
 * =============================================================================
 */

// Good resource for later:
// https://www.singlix.com/trdos/archive/vga/

#include <i386/boot.h>
#include <i386/interrupt.h>
#include <i386/io.h>
#include <i386/paging.h>
#include <i386/x86.h>
#include <kernel/ohwes.h>
#include <kernel/terminal.h>
#include <kernel/vga.h>

// Text mode only for now!

void init_vga(void)
{
    struct vga_fb_info fb_old, fb_new;

    // may get called twice: once during vt_console setup and another
    // during init_terminal to ensure frame buffer is set correctly
    static __initmem bool vga_initialized = false;
    if (vga_initialized) {
        return;
    }

    // set frame buffer address
    vga_get_fb_info(&fb_old);
    vga_set_fb(VGA_MEMORY_128K);    // 32 pages, up to 16 80x50 VTs
    vga_get_fb_info(&fb_new);

    vga_initialized = true;
}

uint8_t vga_get_rows(void)
{
    uint8_t of = vga_crtc_read(VGA_CRTC_REG_OF);
    uint16_t vde = vga_crtc_read(VGA_CRTC_REG_VDE);
    vde |= ((of & VGA_CRTC_FLD_OF_VDE8_MASK) >> VGA_CRTC_FLD_OF_VDE8_SHIFT) << 8;
    vde |= ((of & VGA_CRTC_FLD_OF_VDE9_MASK) >> VGA_CRTC_FLD_OF_VDE9_SHIFT) << 9;
    uint8_t msl = vga_crtc_read(VGA_CRTC_REG_MSL) & VGA_CRTC_FLD_MSL_MSL_MASK;
    return (uint8_t) ((vde + 1) / (msl + 1));
}

uint8_t vga_get_cols(void)
{
    uint8_t hde = vga_crtc_read(VGA_CRTC_REG_HDE);
    return hde + 1;
}

void vga_get_fb_info(struct vga_fb_info *fb_info)
{
    uint8_t grfx_misc;
    uint8_t fb_select;

    assert(fb_info);

    grfx_misc = vga_grfx_read(VGA_GRFX_REG_MISC);
    fb_select = (grfx_misc & 0x0C) >> 2;

    switch (fb_select) {
        case VGA_GRFX_ENUM_MISC_MMAP_128K:
            fb_info->framebuf = 0xA0000;
            fb_info->size_pages = 32;
            break;
        case VGA_GRFX_ENUM_MISC_MMAP_64K:
            fb_info->framebuf = 0xA0000;
            fb_info->size_pages = 16;
            break;
        case VGA_GRFX_ENUM_MISC_MMAP_32K_LO:
            fb_info->framebuf = 0xB0000;
            fb_info->size_pages = 8;
            break;
        case VGA_GRFX_ENUM_MISC_MMAP_32K_HI:
            fb_info->framebuf = 0xB8000;
            fb_info->size_pages = 8;
            break;
    }
}

void vga_set_fb(enum vga_fb_select fb_select)
{
    uint8_t grfx_misc;

    grfx_misc = vga_grfx_read(VGA_GRFX_REG_MISC);
    grfx_misc = (grfx_misc & 0xF3) | ((fb_select & 3) << 2);
    vga_grfx_write(VGA_GRFX_REG_MISC, grfx_misc);

    grfx_misc = vga_grfx_read(VGA_GRFX_REG_MISC);
    assert((grfx_misc & 0x0C) >> 2 == fb_select);
}

void vga_enable_blink(bool enable)
{
    uint32_t flags;
    uint8_t modectl;

    cli_save(flags);
    modectl = vga_attr_read(VGA_ATTR_REG_MODE);
    if (enable) {
        modectl |= VGA_ATTR_FLD_MODE_BLINK;
    }
    else {
        modectl &= ~VGA_ATTR_FLD_MODE_BLINK;
    }
    vga_attr_write(VGA_ATTR_REG_MODE, modectl);
    restore_flags(flags);
}

void vga_enable_cursor(bool enable)
{
    uint32_t flags;
    uint8_t css;

    cli_save(flags);
    css = vga_crtc_read(VGA_CRTC_REG_CSS);
    if (!enable) {
        css |= VGA_CRTC_FLD_CSS_CD_MASK;
    }
    else {
        css &= ~VGA_CRTC_FLD_CSS_CD_MASK;
    }
    vga_crtc_write(VGA_CRTC_REG_CSS, css);
    restore_flags(flags);
}

uint16_t vga_get_cursor_pos(void)
{
    uint32_t flags;

    cli_save(flags);
    uint8_t cl_hi = vga_crtc_read(VGA_CRTC_REG_CL_HI);
    uint8_t cl_lo = vga_crtc_read(VGA_CRTC_REG_CL_LO);
    restore_flags(flags);

    return (cl_hi << 8) | cl_lo;
}

void vga_set_cursor_pos(uint16_t pos)
{
    uint32_t flags;

    cli_save(flags);
    vga_crtc_write(VGA_CRTC_REG_CL_HI, pos >> 8);
    vga_crtc_write(VGA_CRTC_REG_CL_LO, pos & 0xFF);
    restore_flags(flags);
}

uint16_t vga_get_cursor_shape(void)
{
    uint32_t flags;
    uint8_t start, end;

    cli_save(flags);
    start = vga_crtc_read(VGA_CRTC_REG_CSS) & VGA_CRTC_FLD_CSS_CSS_MASK;
    end   = vga_crtc_read(VGA_CRTC_REG_CSE) & VGA_CRTC_FLD_CSE_CSE_MASK;
    restore_flags(flags);

    return (end << 8) | start;
}

void vga_set_cursor_shape(uint16_t shape)
{
    uint32_t flags;
    uint8_t css, cse;
    uint8_t start, end;

    start = shape & 0xFF;
    end = shape >> 8;

    cli_save(flags);
    css = vga_crtc_read(VGA_CRTC_REG_CSS) & ~VGA_CRTC_FLD_CSS_CSS_MASK;
    cse = vga_crtc_read(VGA_CRTC_REG_CSE) & ~VGA_CRTC_FLD_CSE_CSE_MASK;
    css |= (start & VGA_CRTC_FLD_CSS_CSS_MASK);
    cse |= (end   & VGA_CRTC_FLD_CSE_CSE_MASK);
    vga_crtc_write(VGA_CRTC_REG_CSS, css);
    vga_crtc_write(VGA_CRTC_REG_CSE, cse);
    restore_flags(flags);
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
