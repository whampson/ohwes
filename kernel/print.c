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
 *         File: kernel/print.c
 *      Created: July 31, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <stdarg.h>
#include <stdio.h>
#include <console.h>
#include <paging.h>
#include <fs.h>
#include <io.h>
#include <vga.h>

#define KPRINT_BUFSIZ   4096
extern struct vga *g_vga;
extern bool system_console_initialized;

uint8_t early_print_attr = 0x47;    // white on red

static void lazy_init_vga(void)
{
    uint8_t grfx_misc = vga_grfx_read(VGA_GRFX_REG_MISC);
    uint8_t fb_select = (grfx_misc & 0x0C) >> 2;
    switch (fb_select) {
        case VGA_FB_128K:
            g_vga->fb = (void *) __phys_to_virt(0xA0000);
            g_vga->fb_size_pages = 32;
            break;
        case VGA_FB_64K:
            g_vga->fb = (void *) __phys_to_virt(0xA0000);
            g_vga->fb_size_pages = 16;
            break;
        case VGA_FB_32K_LO:
            g_vga->fb = (void *) __phys_to_virt(0xB0000);
            g_vga->fb_size_pages = 8;
            break;
        case VGA_FB_32K_HI:
        default:
            g_vga->fb = (void *) __phys_to_virt(0xB8000);
            g_vga->fb_size_pages = 8;
            break;
    }
}

void early_print(const char *buf, size_t count)
{
    //
    // writes directly to the frame buffer,
    // bypassing all the console and tty gobbledygook
    //
    // set early_print_attr to affect the color attributes
    //
    static int pos = 0; // always prints at top left!
    const int cols = 80;

    if (!g_vga->fb) {
        lazy_init_vga();    // we tried to print before initializing the VGA!
    }

    for (int i = 0; i < count; i++) {
        char c = buf[i];
        if (c == '\0') {
            break;
        }
        uint16_t x = pos % cols;
        uint16_t y = pos / cols;
        if (c == '\n') {
            x = 0; y++; // no scroll support!
        }
        else {
            ((uint16_t *) g_vga->fb)[pos] = (early_print_attr << 8) | c;
            x++;
        }
        pos = y * cols + x;

#if E9_HACK
        outb(0xE9, c);
#endif
        // TODO: write serial port?
    }
}

void write_syscon(const char *buf, size_t count)
{
    if (!system_console_initialized) {
        early_print(buf, count);
        return;
    }

    write_console(get_console(SYSTEM_CONSOLE), buf, count);
}

void write_console(struct console *cons, const char *buf, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        char c = buf[i];
        if (c == '\0') {
            break;
        }
        if (c == '\n') {
            // OPOST && ONLCR    TODO: kprint termios?
            console_putchar(cons, '\r');
        }
        console_putchar(cons, c);
    }
}

//
// prints to the console, bypasses TTY subsystem
//
int _kprint(const char *fmt, ...)
{
    size_t nchars;
    char buf[KPRINT_BUFSIZ];

    va_list args;

    va_start(args, fmt);
    nchars = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    write_syscon(buf, nchars);
    return nchars;
}
