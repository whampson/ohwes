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
 *         File: src/kernel/drivers/char/vga.c
 *      Created: December 14, 2020
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <i386/boot.h>
#include <i386/interrupt.h>
#include <i386/io.h>
#include <i386/paging.h>
#include <i386/x86.h>
#include <kernel/console.h>
#include <kernel/ohwes.h>
#include <kernel/vga.h>

__data_segment static struct vga _vga = { .active_console = SYSTEM_CONSOLE };
__data_segment struct vga *g_vga = &_vga;

void init_vga(void)
{
    struct vga_fb_info fb_info_old, fb_info_new;
    void *fb_old, *fb_new;

    // grab text mode dimensions from boot info
    g_vga->rows = g_boot->vga_rows;
    g_vga->cols = g_boot->vga_cols;
    g_vga->active_console = SYSTEM_CONSOLE;

    // read the current frame buffer parameters
    if (!vga_get_fb_info(&fb_info_old)) {
        panic("failed to get VGA frame buffer info!");
    }
    if (!vga_set_fb(VGA_FB_SELECT)) {
        panic("failed to change VGA frame buffer!");
    }
    if (!vga_get_fb_info(&fb_info_new)) {
        panic("failed to get VGA frame buffer info!");
    }
    g_vga->fb_info = fb_info_new;

    // move the old frame buffer to the new one
    fb_old = (void *) __phys_to_virt(fb_info_old.framebuf);
    fb_new = (void *) __phys_to_virt(fb_info_new.framebuf);
    memmove(fb_new, fb_old, FB_SIZE_PAGES);

    // // update system console frame buffer
    get_console(SYSTEM_CONSOLE)->framebuf = fb_new;

    // read cursor attributes leftover from BIOS
    uint8_t cl_lo, cl_hi;
    uint16_t pos;
    cl_lo = vga_crtc_read(VGA_CRTC_REG_CL_LO);
    cl_hi = vga_crtc_read(VGA_CRTC_REG_CL_HI);
    pos = (cl_hi << 8) | cl_lo;
    g_boot->cursor_col = pos % g_vga->cols;
    g_boot->cursor_row = pos / g_vga->cols;
    cl_lo = vga_crtc_read(VGA_CRTC_REG_CSS) & VGA_CRTC_FLD_CSS_CSS_MASK;
    cl_hi = vga_crtc_read(VGA_CRTC_REG_CSE) & VGA_CRTC_FLD_CSE_CSE_MASK;
    g_vga->orig_cursor_shape = (cl_hi << 8) | cl_lo;

    kprint("frame buffer is VGA, %d pages at %08X\n",
        g_vga->fb_info.size_pages, g_vga->fb_info.framebuf);
}

bool vga_get_fb_info(struct vga_fb_info *fb_info)
{
    uint8_t grfx_misc;
    uint8_t fb_select;

    if (!fb_info) {
        return false;
    }

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
        default:    // cannot happen...
            return false;
    }

    return true;
}

bool vga_set_fb(enum vga_fb_select fb_select)
{
    volatile uint8_t grfx_misc;

    grfx_misc = vga_grfx_read(VGA_GRFX_REG_MISC);
    grfx_misc = (grfx_misc & 0xF3) | ((fb_select & 3) << 2);
    vga_grfx_write(VGA_GRFX_REG_MISC, grfx_misc);

    grfx_misc = vga_grfx_read(VGA_GRFX_REG_MISC);
    return ((grfx_misc & 0x0C) >> 2) == fb_select;
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
