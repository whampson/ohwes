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
 *         File: kernel/kprint.c
 *      Created: April 28, 2025
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <stdarg.h>
#include <stdio.h>
#include <i386/boot.h>
#include <i386/io.h>
#include <i386/paging.h>
#include <kernel/kernel.h>
#include <kernel/fs.h>
#include <kernel/irq.h>
#include <kernel/ohwes.h>
#include <kernel/vga.h>
#include <kernel/serial.h>
#include <kernel/terminal.h>

// TODO: create 'console' struct (separate from the VGA "console") and include
// the following members:
//  char name;
//  dev_t device;
//  int flags;
//  int setup();
//  int read();
//  int write();
//  struct console *next;
//
// register console here and use list for kprint routing. console can be the
// screen/keyboard (terminal) or a serial port.
//
// buffer early output to avoid haphazardly writing to partially-initialized
// hardware

// struct syscon {
//     bool initialized;
//     bool tty_open;
//     bool tried_tty_open;
//     struct tty *tty;
//     int (*init)(void);
//     int (*write)(char *buf);
//     int (*waitkey)(void);
// }


// TODO: this file is incredibly sloppy at the moment...

#define MAX_PRINT_LEN       4096
#define KPRINT_BUFSIZ       MAX_PRINT_LEN
#define PANIC_BUFSIZ        128

__data_segment bool g_earlycons_initialized = false;
__data_segment bool g_tty_initialized = false;
__data_segment bool g_kb_initialized = false;
__data_segment bool g_timer_initialized = false;

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

static void lazy_terminal_init(struct terminal *cons)
{
    struct vga_fb_info fb_info;
    vga_get_fb_info(&fb_info);
    g_vga->rows = g_boot->vga_rows;
    g_vga->cols = g_boot->vga_cols;

    terminal_defaults(cons);
    cons->number = SYSTEM_TERMINAL;
    cons->cols = g_boot->vga_cols;
    cons->rows = g_boot->vga_rows;
    cons->framebuf = (void *) fb_info.framebuf;
    cons->cursor.x = g_boot->cursor_col;
    cons->cursor.y = g_boot->cursor_row;
    g_earlycons_initialized = true;

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

        com_port_initialized = true;
    }
}

int console_print(const char *buf)
{
    struct terminal *cons;
    const char *p;

    cons = get_terminal(SYSTEM_TERMINAL);
    if (!cons->initialized && !g_earlycons_initialized) {
        // we tried to print before the terminal was initialized! do the bare
        // minimum initialization here so we can print safely; full
        // initialization will occur when init_terminal() is called
        lazy_terminal_init(cons);
    }

    if (g_tty_initialized) {
        if (!cons_tty_open && !tried_cons_tty_open) {
            get_tty(__mkcondev(SYSTEM_TERMINAL), &cons_tty);
            cons_tty_open = (tty_open_internal(cons_tty) == 0);
            tried_cons_tty_open = true;
        }
        if (!seri_tty_open && !tried_seri_tty_open) {
            get_tty(__mkserdev(SERIAL_CONSOLE_NUM), &seri_tty);
            seri_tty_open = (tty_open_internal(seri_tty) == 0);
            tried_seri_tty_open = true;
        }
    }

    p = buf;
    while (*p != '\0' && (p - buf) < MAX_PRINT_LEN) {
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
                terminal_putchar(cons, '\r');
            }
            terminal_putchar(cons, *p);
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

    return (p - buf);
}

// int tty_print(struct tty *tty, const char *buf)
// {

// }

int kprint(const char *fmt, ...)
{
    char buf[KPRINT_BUFSIZ+1] = { };
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    return console_print(buf);
}

void __noreturn panic(const char *fmt, ...)
{
    char buf[PANIC_BUFSIZ+1] = { };
    va_list args;

    irq_disable();

    va_start(args, fmt);
    vsnprintf(buf, PANIC_BUFSIZ, fmt, args);
    va_end(args);

    console_print("\n\e[1;31mpanic: ");
    console_print(buf);
    console_print("\e[0m");

    if (g_kb_initialized || g_timer_initialized) {
        irq_setmask(IRQ_MASKALL);
        if (g_kb_initialized) {
            irq_unmask(IRQ_KEYBOARD);   // TODO: disable echo
        }
        if (g_timer_initialized) {
            irq_unmask(IRQ_TIMER);
        }
        irq_enable();
    }

    for (;;);
}

void print_boot_info(void)
{
    int nfloppies = g_boot->hwflags.has_diskette_drive;
    if (nfloppies) {
        nfloppies += g_boot->hwflags.num_other_diskette_drives;
    }

    int nserial = g_boot->hwflags.num_serial_ports;
    int nparallel = g_boot->hwflags.num_parallel_ports;
    bool gameport = g_boot->hwflags.has_gameport;
    bool mouse = g_boot->hwflags.has_ps2mouse;
    uint32_t ebda_size = 0xA0000 - g_boot->ebda_base;

    kprint("bios-boot: %d %s, %d serial %s, %d parallel %s\n",
        nfloppies, PLURAL2(nfloppies, "floppy", "floppies"),
        nserial, PLURAL(nserial, "port"),
        nparallel, PLURAL(nparallel, "port"));
    kprint("bios-boot: A20 mode is %s\n",
        (g_boot->a20_method == A20_KEYBOARD) ? "A20_KEYBOARD" :
        (g_boot->a20_method == A20_PORT92) ? "A20_PORT92" :
        (g_boot->a20_method == A20_BIOS) ? "A20_BIOS" :
        "A20_NONE");
    kprint("bios-boot: %s PS/2 mouse, %s game port\n", HASNO(mouse), HASNO(gameport));
    kprint("bios-boot: video mode is %02Xh\n", g_boot->vga_mode & 0x7F);
    if (g_boot->ebda_base) kprint("bios-boot: EBDA=%08X,%Xh\n", g_boot->ebda_base, ebda_size);
}
