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
 *      Created: May 12, 2026
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <stdarg.h>
#include <stdio.h>
#include <i386/io.h>
#include <kernel/console.h>
#include <kernel/kernel.h>
#include <kernel/irq.h>
#include <kernel/mm.h>

#define KPRINT_MAX  BUFSIZ

static int _log_start = 0;
static int _log_size = 0;
static char *_kernel_log = (char *) __klog;

static char _kprint_buf[KPRINT_MAX+1] = { };

// #if KPRINT_TIME
// static bool kprint_time = true;
// #else
// static bool kprint_time = false;
// #endif

#if E9_HACK
static dev_t e9_console_device(struct console *cons)
{
    // Hmm... not sure about this yet...
    // it's a hack; not a 'real' console so maybe the device ID is irrelevant?
    return __mkdev(0, 0);
}

static bool e9_console_setup(struct console *cons)
{
    return true;
}

static ssize_t e9_console_write(struct console *cons, const char *buf, size_t count)
{
    const char *p = buf;
    while ((p - buf) < count) {
        outb(0xE9, *p++);
    }

    return (p - buf);
}

static int e9_console_read_char(struct console *cons)
{
    return 0;
}

struct console e9_console =
{
    .name = "e9hack",
    .number = 0,
    .flags = _CONSOLE_FLAG_PRINTBUF,
    .device = e9_console_device,
    .init = e9_console_setup,
    .write = e9_console_write,
    .read_char = e9_console_read_char,
};
#endif

static int klog_write(const char *buf, size_t count)
{
    const char *p;
    const char *line;
    int linefeed;
    struct console *cons;

#if E9_HACK
    static bool e9_console_registered = false;
    if (!e9_console_registered) {
        register_console(&e9_console);
        e9_console_registered = true;
    }
#endif

#if EARLY_PRINT
    static bool early_cons_registered = false;
    if (!early_cons_registered) {
  #if VT_CONSOLE
        extern struct console vt_console;       // see char/terminal.c
        register_console(&vt_console);
  #elif SERIAL_CONSOLE
        extern struct console serial_console;   // see char/serial.c
        register_console(&serial_console);
  #else
    #error "No console enabled for early print!"
  #endif
        early_cons_registered = true;
    }
#endif

    if (count > KPRINT_MAX) {
        count = KPRINT_MAX;
    }

    p = buf;
    linefeed = 0;
    while ((p - buf) < count && *p != '\0') {
        // TODO: figure out log level

        // add to the log 'til we see a linefeed or hit the end
        for (line = p; (p - buf) < count && *p != '\0'; p++) {
            _kernel_log[(_log_start + _log_size) % KERNEL_LOG_SIZE] = *p;
            if (_log_size < KERNEL_LOG_SIZE) {
                _log_size++;
            }
            else {
                _log_start += 1;
                _log_start %= KERNEL_LOG_SIZE;
            }
            linefeed = (*p == '\n');
            if (linefeed) {
                break;
            }
        }

        // write line to console
        cons = g_console_list;
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

void klog_flush_to_console(struct console *cons)
{
    const char *log_ptr = &_kernel_log[_log_start];
    if (_log_size < KERNEL_LOG_SIZE) {
        cons->write(cons, log_ptr, _log_size);
    }
    else {
        cons->write(cons, log_ptr, KERNEL_LOG_SIZE - _log_start);
        cons->write(cons, _kernel_log, _log_start);
    }
}

int kprint(const char *fmt, ...)
{
    size_t count, n;

    va_list args;
    va_start(args, fmt);

    // TODO: buffer log messages, store timestamp, log level, and message.
    // if call to kprint does not contain a newline, buffer line for some time...
    // after some time, if newline doesn't appear, or KERN_CONT does not appear, flush to log

    uint64_t ns = get_uptime();
    uint32_t sec = (uint32_t) (ns / 1000000000);
    uint32_t micros = (uint32_t) ((ns % 1000000000) / 1000);

    n = snprintf(_kprint_buf, KPRINT_MAX, "[%5lu.%06lu] ", sec, micros);
    n += vsnprintf(_kprint_buf+n, KPRINT_MAX-n, fmt, args);
    count = klog_write(_kprint_buf, n);

    va_end(args);
    return count;
}

void __noreturn panic(const char *fmt, ...)
{
    size_t n;

    va_list args;
    va_start(args, fmt);

    n = snprintf(_kprint_buf, KPRINT_MAX, "\n\e[1;31mpanic: ");
    n += vsnprintf(_kprint_buf+n, KPRINT_MAX-n, fmt, args);
    n += snprintf(_kprint_buf+n, KPRINT_MAX-n, "\e[0m");
    klog_write(_kprint_buf, n);

    va_end(args);

    irq_disable();
    irq_setmask(IRQ_MASKALL);

#if SERIAL_DEBUGGING
    if (SERIAL_DEBUG_PORT == COM1_PORT || SERIAL_DEBUG_PORT == COM3_PORT) {
        irq_unmask(IRQ_COM1);
    }
    else {
        irq_unmask(IRQ_COM2);
    }
#endif

    irq_unmask(IRQ_TIMER);

    extern bool g_kb_initialized;
    if (g_kb_initialized) {
        irq_unmask(IRQ_KEYBOARD);
    }

    irq_enable();

#if SERIAL_DEBUGGING
    __int3();
#endif

    for (;;);
}
