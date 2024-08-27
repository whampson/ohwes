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
#include <fs.h>
#include <paging.h>

#define KPRINT_BUFSIZ   4096

uint8_t early_print_attr = 0x07;

extern bool system_console_initialized;

void early_print(const char *buf, size_t count)
{
    static int pos = 0; // always prints at top left!
    const int cols = 80;

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
            ((uint16_t *) __phys_to_virt(0xB8000))[pos] = (early_print_attr << 8) | c;
            x++;
        }
        pos = y * cols + x;
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
