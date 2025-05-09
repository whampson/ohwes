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
#include <kernel/console.h>
#include <kernel/kernel.h>
#include <kernel/fs.h>
#include <kernel/irq.h>
#include <kernel/ohwes.h>
#include <kernel/vga.h>
#include <kernel/serial.h>
#include <kernel/terminal.h>
#include <kernel/queue.h>

#define KLOG_BUFSIZ         4096
#define KPRINT_BUFSIZ       KLOG_BUFSIZ
#define PANIC_BUFSIZ        128

#define __initmem __data_segment

__initmem static int _log_start = 0;
__initmem static int _log_size = 0;

// TODO: find a way to store this neither in init memory, nor in BSS;
// we don't want to zero this buffer when BSS is initialized, but we also
// don't want 4K of empty space occupying our kernel image!
__initmem static char _kernel_log[KLOG_BUFSIZ];

__initmem static struct console *consoles = NULL;

// TODO: remove
__data_segment bool g_kb_initialized = false;
__data_segment bool g_timer_initialized = false;

int kprint(const char *fmt, ...)
{
    char buf[KPRINT_BUFSIZ+1] = { };
    va_list args;
    int count;
    int linefeed;

    const char *p;
    const char *line;
    struct console *cons;

    va_start(args, fmt);
    count = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    p = buf;
    linefeed = 0;
    while ((p - buf) < count) {
        // add to the log 'til we see a linefeed or hit the end
        for (line = p; (p - buf) < count; p++) {
            _kernel_log[(_log_start + _log_size) % KLOG_BUFSIZ] = *p;
            if (_log_size < KLOG_BUFSIZ) {
                _log_size++;
            }
            else {
                _log_start += 1;
                _log_start %= KLOG_BUFSIZ;
            }
            linefeed = (*p == '\n');
            if (linefeed) {
                break;
            }
        }

        // write line to console
        cons = consoles;
        while (cons) {
            if (cons->write) {
                cons->write(cons, line, (p - line) + linefeed);
            }
            cons = cons->next;
        }

        if (linefeed) {
            linefeed = 0;
            p++;
        }
    }

    return (p - buf);
}

void register_console(struct console *cons)
{
    int ret;
    const char *log_ptr;
    struct console *currcons;

    assert(cons);

    if (consoles == NULL) {
        consoles = cons;
        cons->next = NULL;
    }
    else {
        currcons = consoles;
        while (currcons->next) {
            currcons = currcons->next;
        }
        currcons->next = cons;
        cons->next = NULL;
    }

    if (cons->setup) {
        ret = cons->setup(cons);
        if (ret != 0) {
            panic("%s setup failed (error %d)", cons->name, ret);
        }
    }
    if (cons->write) {
        log_ptr = &_kernel_log[_log_start];
        if (_log_size < KLOG_BUFSIZ) {
            cons->write(cons, log_ptr, _log_size);
        }
        else {
            cons->write(cons, log_ptr, KLOG_BUFSIZ - _log_start);
            cons->write(cons, _kernel_log, _log_start);
        }
    }
}
void unregister_console(struct console *cons)
{
    struct console *currcons;
    struct console *prevcons;

    assert(cons);

    if (consoles == cons) {
        consoles = consoles->next;
        cons->next = NULL;
        return;
    }

    currcons = consoles;
    while (currcons->next) {
        prevcons = currcons;
        currcons = currcons->next;
        if (currcons == cons) {
            prevcons->next = currcons->next;
            currcons = NULL;
            break;
        }
    }
}

static void print_log(void)    // TODO: procfs for this
{
    for (int i = 0; i < _log_size; i++) {
        // printf("%c", _kernel_log[(_log_start + i) % _log_size]);
    }
}

static void print_consoles(void)   // TODO: procfs for this
{
    struct console *cons;

    cons = consoles;
    while (cons) {
        // printf("%s ", cons->name);
        cons = cons->next;
    }
    // printf("\n");
}


// inline static char com_in(int port)
// {
//     return inb(SERIAL_OUTPUT_PORT + port);
// }

// inline static void com_out(int port, char data)
// {
//     outb(SERIAL_OUTPUT_PORT + port, data);
// }

// static void com_putc(char c)
// {
//     while ((com_in(UART_LSR) & UART_LSR_THRE) == 0);
//     com_out(UART_TX, c);
// }

// extern int tty_open_internal(struct tty *tty);

// static void lazy_terminal_init(struct terminal *term)
// {
//     // struct vga_fb_info fb_info;
//     // vga_get_fb_info(&fb_info);
//     // g_vga->rows = g_boot->vga_rows;
//     // g_vga->cols = g_boot->vga_cols;

//     // terminal_defaults(term);
//     // term->number = SYSTEM_TERMINAL;
//     // term->cols = g_boot->vga_cols;
//     // term->rows = g_boot->vga_rows;
//     // term->framebuf = (void *) fb_info.framebuf;
//     // term->cursor.x = g_boot->cursor_col;
//     // term->cursor.y = g_boot->cursor_row;
//     // g_earlycons_initialized = true;

//     struct lcr lcr = {
//         .word_length = WLS_8,       // 8 data bits
//         .stop_bits   = STB_1,       // 1 stop bit
//         .parity      = PARITY_NONE, // no parity
//     };
//     struct mcr mcr = {
//         .dtr        = 1,            // data terminal ready
//         .rts        = 1,            // request to send
//         .out2       = 1,            // carrier detect
//     };
//     struct ier ier = {
//         .rda        = 1,            // data ready
//         .rls        = 1,            // line status interrupt
//         .ms         = 1,            // modem status interrupt
//     };
//     uint16_t baud   = BAUD_9600;

//     com_out(UART_SCR, 0);
//     com_out(UART_SCR, 0x55);
//     if (com_in(UART_SCR) == 0x55) {
//         com_out(UART_IER, ier._value);
//         com_out(UART_MCR, mcr._value);
//         com_out(UART_LCR, UART_LCR_DLAB);
//         com_out(UART_DLM, baud >> 8);
//         com_out(UART_DLL, baud & 0xFF);
//         com_out(UART_LCR, lcr._value);
//         com_out(UART_FCR, 0);

//         (void) com_in(UART_LSR);
//         (void) com_in(UART_MSR);
//         (void) com_in(UART_IIR);

//         com_port_initialized = true;
//     }
// }

// int console_print(const char *buf)
// {
//     // struct terminal *term;
//     // const char *p;

//     // term = get_terminal(SYSTEM_TERMINAL);
//     // if (!term->initialized && !g_earlycons_initialized) {
//     //     // we tried to print before the terminal was initialized! do the bare
//     //     // minimum initialization here so we can print safely; full
//     //     // initialization will occur when init_terminal() is called
//     //     lazy_terminal_init(term);
//     // }

//     // if (g_tty_initialized) {
//     //     if (!cons_tty_open && !tried_cons_tty_open) {
//     //         get_tty(__mkcondev(SYSTEM_TERMINAL), &cons_tty);
//     //         cons_tty_open = (tty_open_internal(cons_tty) == 0);
//     //         tried_cons_tty_open = true;
//     //     }
//     //     if (!seri_tty_open && !tried_seri_tty_open) {
//     //         get_tty(__mkserdev(SERIAL_CONSOLE_NUM), &seri_tty);
//     //         seri_tty_open = (tty_open_internal(seri_tty) == 0);
//     //         tried_seri_tty_open = true;
//     //     }
//     // }

//     // p = buf;
//     // while (*p != '\0' && (p - buf) < MAX_PRINT_LEN) {
//     //     if (seri_tty_open) {
//     //         tty_putchar(seri_tty, *p);
//     //     }
//     //     else if (com_port_initialized) {
//     //         if (*p == '\n') {
//     //             com_putc('\r');
//     //         }
//     //         com_putc(*p);
//     //     }
//     //     if (cons_tty_open) {
//     //        tty_putchar(cons_tty, *p);
//     //     }
//     //     else {
//     //         if (*p == '\n') {
//     //             terminal_putchar(term, '\r');
//     //         }
//     //         terminal_putchar(term, *p);
//     //     }
//     //     p++;
//     // }

//     // if (seri_tty_open) {
//     //     tty_flush(seri_tty);
//     // }
//     // if (cons_tty_open) {
//     //     tty_flush(cons_tty);
//     // }

//     // g_boot->cursor_col = term->cursor.x;
//     // g_boot->cursor_row = term->cursor.y;

//     // return (p - buf);
// }

// int tty_print(struct tty *tty, const char *buf)
// {

// }


void __noreturn panic(const char *fmt, ...)
{
    char buf[PANIC_BUFSIZ+1] = { };
    va_list args;

    irq_disable();

    va_start(args, fmt);
    vsnprintf(buf, PANIC_BUFSIZ, fmt, args);
    va_end(args);

    kprint("\n\e[1;31mpanic: ");
    kprint(buf);
    kprint("\e[0m");

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
