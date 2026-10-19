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
#include <kernel/kernel.h>
#include <kernel/terminal.h>
#include <kernel/vga.h>

// Text mode only for now!

uint16_t vga_get_rows(void)
{
    uint16_t vde;
    uint8_t of, msl;
    uint8_t crtc[] = { VGA_CRTC_REG_OF, VGA_CRTC_REG_MSL, VGA_CRTC_REG_VDE };
    uint8_t data[3];

    vga_crtc_read_n(crtc, data, countof(crtc));
    of = data[0];
    msl = data[1] & VGA_CRTC_FLD_MSL_MSL_MASK;
    vde = data[2]
        | (((of & VGA_CRTC_FLD_OF_VDE8_MASK) >> VGA_CRTC_FLD_OF_VDE8_SHIFT) << 8)
        | (((of & VGA_CRTC_FLD_OF_VDE9_MASK) >> VGA_CRTC_FLD_OF_VDE9_SHIFT) << 9);

    return ((vde + 1) / (msl + 1));
}

uint16_t vga_get_cols(void)
{
    uint8_t hde = vga_crtc_read(VGA_CRTC_REG_HDE);
    return hde + 1;
}

void vga_get_fb_info(struct vga_fb_info *fb_info)
{
    uint32_t flags;
    uint8_t grfx_misc;
    uint8_t fb_select;

    assert(fb_info);

    cli_save(flags);
    grfx_misc = vga_grfx_read(VGA_GRFX_REG_MISC);
    fb_select = (grfx_misc & VGA_GRFX_FLD_MISC_MMAP_MASK) >> 2;
    restore_flags(flags);

    switch (fb_select) {
        case VGA_GRFX_ENUM_MISC_MMAP_128K:
            fb_info->phys_ptr = 0xA0000;
            fb_info->size_pages = 32;
            break;
        case VGA_GRFX_ENUM_MISC_MMAP_64K:
            fb_info->phys_ptr = 0xA0000;
            fb_info->size_pages = 16;
            break;
        case VGA_GRFX_ENUM_MISC_MMAP_32K_LO:
            fb_info->phys_ptr = 0xB0000;
            fb_info->size_pages = 8;
            break;
        case VGA_GRFX_ENUM_MISC_MMAP_32K_HI:
        default:    // 2-bit fallback
            fb_info->phys_ptr = 0xB8000;
            fb_info->size_pages = 8;
            break;
    }

    fb_info->ptr = KERNEL_ADDR(fb_info->phys_ptr);
}

void vga_set_fb_base(enum vga_fb_select fb_select)
{
    uint32_t flags;
    uint8_t grfx_misc;

    cli_save(flags);
    grfx_misc = vga_grfx_read(VGA_GRFX_REG_MISC);
    grfx_misc = (grfx_misc & 0xF3) | ((fb_select & 3) << 2);
    vga_grfx_write(VGA_GRFX_REG_MISC, grfx_misc);

    grfx_misc = vga_grfx_read(VGA_GRFX_REG_MISC);
    assert((grfx_misc & 0x0C) >> 2 == fb_select);
    restore_flags(flags);
}

void vga_set_screen_blank(bool blank)
{
    uint32_t flags;
    uint8_t mode;

    cli_save(flags);
    mode = vga_seqr_read(VGA_SEQR_REG_CLOCKING);
    if (blank) {
        mode |= VGA_SEQR_FLD_CLOCKING_SO;
    }
    else {
        mode &= ~VGA_SEQR_FLD_CLOCKING_SO;
    }
    vga_seqr_write(VGA_SEQR_REG_CLOCKING, mode);
    restore_flags(flags);
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
    uint8_t crtc[] = { VGA_CRTC_REG_CL_HI, VGA_CRTC_REG_CL_LO };
    uint8_t data[2];

    vga_crtc_read_n(crtc, data, countof(crtc));

    return (data[0] << 8) | data[1];
}

void vga_set_cursor_pos(uint16_t pos)
{
    uint8_t crtc[] = { VGA_CRTC_REG_CL_HI, VGA_CRTC_REG_CL_LO };
    uint8_t data[] = { pos >> 8,           pos & 0xFF };

    vga_crtc_write_n(crtc, data, countof(crtc));
}

uint16_t vga_get_cursor_shape(void)
{
    uint8_t start, end;
    uint8_t crtc[] = { VGA_CRTC_REG_CSE, VGA_CRTC_REG_CSS };
    uint8_t data[2];

    vga_crtc_read_n(crtc, data, countof(crtc));
    end   = data[0] & VGA_CRTC_FLD_CSE_CSE_MASK;
    start = data[1] & VGA_CRTC_FLD_CSS_CSS_MASK;

    return (end << 8) | start;
}

void vga_set_cursor_shape(uint16_t shape)
{
    uint8_t crtc[] = { VGA_CRTC_REG_CSE, VGA_CRTC_REG_CSS };
    uint8_t data[2];

    vga_crtc_read_n(crtc, data, countof(crtc));
    data[0] &= ~VGA_CRTC_FLD_CSE_CSE_MASK;
    data[1] &= ~VGA_CRTC_FLD_CSS_CSS_MASK;
    data[0] |= ((shape >> 8)   & VGA_CRTC_FLD_CSE_CSE_MASK);    // end
    data[1] |= ((shape & 0xFF) & VGA_CRTC_FLD_CSS_CSS_MASK);    // start
    vga_crtc_write_n(crtc, data, countof(crtc));
}

uint16_t vga_get_scan_start(void)
{
    uint8_t crtc[] = { VGA_CRTC_REG_ADDR_HI, VGA_CRTC_REG_ADDR_LO };
    uint8_t data[2];

    vga_crtc_read_n(crtc, data, countof(crtc));

    return (data[0] << 8) | data[1];
}

void vga_set_scan_start(uint16_t word_offset)
{
    uint8_t crtc[] = { VGA_CRTC_REG_ADDR_HI, VGA_CRTC_REG_ADDR_LO };
    uint8_t data[] = { word_offset >> 8,     word_offset & 0xFF };

    vga_crtc_write_n(crtc, data, countof(crtc));
}

uint16_t vga_get_line_compare(void)
{
    uint16_t scan_line;
    uint8_t lc8_mask, lc9_mask;
    uint8_t crtc[] = { VGA_CRTC_REG_OF, VGA_CRTC_REG_MSL, VGA_CRTC_REG_LC };
    uint8_t data[3];

    vga_crtc_read_n(crtc, data, countof(crtc));
    lc8_mask = data[0] & VGA_CRTC_FLD_OF_LC8_MASK;
    lc9_mask = data[1] & VGA_CRTC_FLD_MSL_LC9_MASK;
    scan_line = data[2] | ((lc8_mask >> VGA_CRTC_FLD_OF_LC8_SHIFT) << 8)
                        | ((lc9_mask >> VGA_CRTC_FLD_MSL_LC9_SHIFT) << 9);

    return scan_line;
}

void vga_set_line_compare(uint16_t scan_line)
{
    uint8_t crtc[] = { VGA_CRTC_REG_OF, VGA_CRTC_REG_MSL, VGA_CRTC_REG_LC };
    uint8_t data[3];

    vga_crtc_read_n(crtc, data, countof(crtc));
    data[0] = (data[0] & ~VGA_CRTC_FLD_OF_LC8_MASK)
            | (((scan_line >> 8) & 1) << VGA_CRTC_FLD_OF_LC8_SHIFT);
    data[1] = (data[1] & ~VGA_CRTC_FLD_MSL_LC9_MASK)
            | (((scan_line >> 9) & 1) << VGA_CRTC_FLD_MSL_LC9_SHIFT);
    data[2] = scan_line & 0xFF;
    vga_crtc_write_n(crtc, data, countof(crtc));
}

void vga_crtc_read_n(const uint8_t *regs, uint8_t *values, size_t n)
{
    uint32_t flags;

    cli_save(flags);
    for (size_t i = 0; i < n; i++) {
        outb(VGA_CRTC_PORT_ADDR, regs[i]);          // TODO: use IOAS! (XOR 6)
        values[i] = inb(VGA_CRTC_PORT_DATA);
    }
    restore_flags(flags);
}

void vga_crtc_write_n(const uint8_t *regs, const uint8_t *values, size_t n)
{
    uint32_t flags;

    cli_save(flags);
    for (size_t i = 0; i < n; i++) {
        outb(VGA_CRTC_PORT_ADDR, regs[i]);
        outb(VGA_CRTC_PORT_DATA, values[i]);
    }
    restore_flags(flags);
}

uint8_t vga_crtc_read(uint8_t reg)
{
    uint32_t flags;
    uint8_t data;

    cli_save(flags);
    outb(VGA_CRTC_PORT_ADDR, reg);
    data = inb(VGA_CRTC_PORT_DATA);
    restore_flags(flags);

    return data;
}

void vga_crtc_write(uint8_t reg, uint8_t data)
{
    uint32_t flags;

    cli_save(flags);
    outb(VGA_CRTC_PORT_ADDR, reg);
    outb(VGA_CRTC_PORT_DATA, data);
    restore_flags(flags);
}

uint8_t vga_grfx_read(uint8_t reg)
{
    uint32_t flags;
    uint8_t data;

    cli_save(flags);
    outb(VGA_GRFX_PORT_ADDR, reg);
    data = inb(VGA_GRFX_PORT_DATA);
    restore_flags(flags);

    return data;
}

void vga_grfx_write(uint8_t reg, uint8_t data)
{
    uint32_t flags;

    cli_save(flags);
    outb(VGA_GRFX_PORT_ADDR, reg);
    outb(VGA_GRFX_PORT_DATA, data);
    restore_flags(flags);
}

uint8_t vga_seqr_read(uint8_t reg)
{
    uint32_t flags;
    uint8_t data;

    cli_save(flags);
    outb(VGA_SEQR_PORT_ADDR, reg);
    data = inb(VGA_SEQR_PORT_DATA);
    restore_flags(flags);

    return data;
}

void vga_seqr_write(uint8_t reg, uint8_t data)
{
    uint32_t flags;

    cli_save(flags);
    outb(VGA_SEQR_PORT_ADDR, reg);
    outb(VGA_SEQR_PORT_DATA, data);
    restore_flags(flags);
}

uint8_t vga_attr_read(uint8_t reg)
{
    uint32_t flags;
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
    uint32_t flags;
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
