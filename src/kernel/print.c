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
 *         File: src/kernel/print.c
 *      Created: July 31, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <stdarg.h>
#include <stdio.h>
#include <i386/boot.h>
#include <i386/io.h>
#include <i386/paging.h>
#include <kernel/console.h>
#include <kernel/console.h>
#include <kernel/fs.h>
#include <kernel/vga.h>

#define KPRINT_BUFSIZ   4096

void print_to_syscon(const char *buf, size_t count)
{
    struct console *cons;
    const char *p;

    cons = get_console(SYSTEM_CONSOLE);
    if (!cons->initialized && !g_early_console_initialized) {
        // we tried to print before the console was initialized! do the bare
        // minimum initialization here so we can print safely; full
        // initialization will occur when init_console() is called
        struct vga_fb_info fb_info;
        vga_get_fb_info(&fb_info);
        g_vga->rows = g_boot->vga_rows;
        g_vga->cols = g_boot->vga_cols;
        g_vga->active_console = SYSTEM_CONSOLE;

        console_defaults(cons);
        cons->number = SYSTEM_CONSOLE;
        cons->cols = g_boot->vga_cols;
        cons->rows = g_boot->vga_rows;
        cons->framebuf = (void *) fb_info.framebuf;
        cons->cursor.x = g_boot->cursor_col;
        cons->cursor.y = g_boot->cursor_row;
        g_early_console_initialized = true;
    }

    p = buf;
    while (p < buf + count) {
        if (*p == '\n') {
            // if console is busy, keep trying til it's not...
            // TODO: THIS IS BAD for interrupt handlers. We need to
            // move the write into a deferred routine. Is this how
            // the TTY flip buffer thing works on linux?
            // TODO: kprint ldisc/termios?
            while (console_putchar(cons, '\r') == 0);
        }
        p += console_putchar(cons, *p);
    }

    g_boot->cursor_col = cons->cursor.x;
    g_boot->cursor_row = cons->cursor.y;
}

//
// prints directly to the system console; bypasses TTY subsystem
//
int _kprint(const char *fmt, ...)
{
    size_t nchars;
    char buf[KPRINT_BUFSIZ];

    va_list args;

    va_start(args, fmt);
    nchars = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    print_to_syscon(buf, nchars);
    return nchars;
}
