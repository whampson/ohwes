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
#include <kernel/serial.h>

// TODO: this file is incredibly sloppy at the moment...

#define KPRINT_BUFSIZ       4096
#define SERIAL_OUTPUT_PORT  COM2_PORT

extern bool g_early_console_initialized;
extern bool g_tty_initialized;

__data_segment static bool cons_tty_open = false;
__data_segment static bool seri_tty_open = false;

__data_segment static bool tried_cons_tty_open = false;
__data_segment static bool tried_seri_tty_open = false;

__data_segment static bool com_port_initialized = false;

static struct tty *seri_tty;
static struct tty *cons_tty;

inline static char com_in(int port)
{
    return inb(SERIAL_OUTPUT_PORT + port);
}

inline static void com_out(int port, char data)
{
    outb(SERIAL_OUTPUT_PORT + port, data);
}

static void com_putc(char c)
{
    while ((com_in(UART_LSR) & UART_LSR_THRE) == 0);
    com_out(UART_TX, c);
}

extern int tty_open_internal(struct tty *tty);

static void lazy_console_init(struct console *cons)
{
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

    struct lcr lcr = {
        .word_length = WLS_8,       // 8 data bits
        .stop_bits   = STB_1,       // 1 stop bit
        .parity      = PARITY_NONE, // no parity
    };
    struct mcr mcr = {
        .dtr        = 1,            // data terminal ready
        .rts        = 1,            // request to send
        .out2       = 1,            // carrier detect
    };
    struct ier ier = {
        .rda        = 1,            // data ready
        .rls        = 1,            // line status interrupt
        .ms         = 1,            // modem status interrupt
    };
    uint16_t baud   = BAUD_9600;

    com_out(UART_SCR, 0);
    com_out(UART_SCR, 0x55);
    if (com_in(UART_SCR) == 0x55) {
        com_out(UART_IER, ier._value);
        com_out(UART_MCR, mcr._value);
        com_out(UART_LCR, UART_LCR_DLAB);
        com_out(UART_DLM, baud >> 8);
        com_out(UART_DLL, baud & 0xFF);
        com_out(UART_LCR, lcr._value);
        com_out(UART_FCR, 0);

        (void) com_in(UART_LSR);
        (void) com_in(UART_MSR);
        (void) com_in(UART_IIR);
        com_port_initialized  = true;
    }
}

void print_to_syscon(const char *buf, size_t count)
{
    struct console *cons;
    const char *p;

    cons = get_console(SYSTEM_CONSOLE);
    if (!cons->initialized && !g_early_console_initialized) {
        // we tried to print before the console was initialized! do the bare
        // minimum initialization here so we can print safely; full
        // initialization will occur when init_console() is called
        lazy_console_init(cons);
    }

    if (g_tty_initialized) {
        if (!cons_tty_open && !tried_cons_tty_open) {
            get_tty(__mkcondev(SYSTEM_CONSOLE), &cons_tty);
            cons_tty_open = (tty_open_internal(cons_tty) == 0);
            tried_cons_tty_open = true;
        }
        if (!seri_tty_open && !tried_seri_tty_open) {
            get_tty(__mkserdev(2), &seri_tty);
            seri_tty_open = (tty_open_internal(seri_tty) == 0);
            tried_seri_tty_open = true;
        }
    }

    p = buf;
    while (p < buf + count) {
        if (seri_tty_open) {
            tty_putchar(seri_tty, *p);
        }
        else if (com_port_initialized) {
            if (*p == '\n') {
                com_putc('\r');
            }
            com_putc(*p);
        }
        if (cons_tty_open) {
           tty_putchar(cons_tty, *p);
        }
        else {
            if (*p == '\n') {
                console_putchar(cons, '\r');
            }
            console_putchar(cons, *p);
        }
        p++;
    }

    if (seri_tty_open) {
        tty_flush(seri_tty);
    }
    if (cons_tty_open) {
        tty_flush(cons_tty);
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
