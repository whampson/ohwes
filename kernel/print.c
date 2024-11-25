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
#include <boot.h>
#include <console.h>
#include <fs.h>
#include <io.h>
#include <vga.h>

#define KPRINT_BUFSIZ   4096

extern struct vga *g_vga;
extern struct boot_info *g_boot;
extern struct console g_console[NR_CONSOLE];

void print_to_console(struct console *cons, const char *buf, size_t count)
{
    if (!cons) {
        panic("console is null!");
    }

    if (!cons->initialized) {
        panic("attempt to print to an uninitialized console!");
    }

    if (!buf) {
        panic("attempt to print from a null buffer!");
        return;
    }

    for (size_t i = 0; i < count; i++) {
        char c = buf[i];
        if (c == '\0') {
            break;
        }
        if (c == '\n') {
            // OPOST && ONLCR
            // TODO: kprint termios?
            console_putchar(cons, '\r');
        }
        console_putchar(cons, c);
    }
}

void print_to_syscon(const char *buf, size_t count)
{
    if (!g_console->initialized) {
        // we tried to print before the console was initialized! do the bare
        // minimum initialization here so we can print safely; full
        // initialization will occur when init_console() is called
        struct vga_fb_info fb_info;
        vga_get_fb_info(&fb_info);
        console_defaults(g_console);
        g_console->cols = g_boot->vga_cols;
        g_console->rows = g_boot->vga_rows;
        g_console->framebuf = (void *) fb_info.framebuf;
        g_console->cursor.x = g_boot->cursor_col;
        g_console->cursor.y = g_boot->cursor_row;
        g_console->initialized = true; // partially true...
    }

    // print using console subsystem
    print_to_console(g_console, buf, count);

    g_boot->cursor_col = g_console->cursor.x;
    g_boot->cursor_row = g_console->cursor.y;
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
