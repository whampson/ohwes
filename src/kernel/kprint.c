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

// This is basically just Linux's printk code. Sue me, Linus!

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <i386/io.h>
#include <kernel/console.h>
#include <kernel/kernel.h>
#include <kernel/irq.h>
#include <kernel/mm.h>

#define LOG_LEVEL_MAX  5    // <5> = LOG_DEBUG

// TODO: configurable
const char* _klog_console_level_prefix[LOG_LEVEL_MAX+1] =
{
/* KLOG_FATAL */ "\e[1;31m\a\a\a",
/* KLOG_ERROR */ "\e[1;31m",
/* KLOG_ALERT */ "\e[1;33m\a",
/* KLOG_WARN  */ "\e[1;33m",
/* KLOG_INFO  */ "\e[37m",
/* KLOG_DEBUG */ "\e[37m",
};
const char *_klog_console_level_suffix = "\e[0m";
const char *_klog_console_time_prefix = "\e[32m";

static char _kprint_buf[BUFSIZ+1] = { };

#if KPRINT_TIME
static bool _kprint_time = true;
#else
static bool _kprint_time = false;
#endif

static unsigned int _klog_tail = 0;
static unsigned int _klog_count = 0;
static const unsigned int _klog_level = DEFAULT_LOG_LEVEL; // TODO: control this somehow
static unsigned int _klog_curr_level = _klog_level;

struct console *g_consoles = NULL;  // linked list

// ----------------------------------------------------------------------------

#if E9_HACK
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
    return 0;   // not supported
}

static bool e9_console_setup(struct console *cons)
{
    e9_console_write(cons, "\r\n", 2);
    return true;
}

static dev_t e9_console_device(struct console *cons)
{
    // Hmm... not sure about this yet...
    // it's a hack; not a 'real' console so maybe the device ID is irrelevant?
    return __mkdev(0, 0);
}

struct console e9_console =
{
    .name = "e9cons",
    .number = 0,
    .flags = _CONSOLE_FLAG_PRINTBUF,
    .device = e9_console_device,
    .init = e9_console_setup,
    .write = e9_console_write,
    .read_char = e9_console_read_char,
};
#endif

// ----------------------------------------------------------------------------

static int klog_parse_prefix(const char *buf, unsigned int *level, char *special)
{
    int len = 0;
    int lev = 0;
    char sp = 0;

    if (buf[0] != '<' || buf[1] == '\0' || buf[2] != '>') {
        return 0;
    }

    switch (buf[1]) {
        case 'c': __fallthrough;
        case 'd':
            sp = buf[1];
            break;
        default:
            if (!isdigit(buf[1])) {
                return 0;
            }
            lev = (buf[1] - '0');
            if (lev > LOG_LEVEL_MAX) {
                return 0;
            }
            break;
    }
    len = 3;

    if (!special && sp) {
        return 0;       // reject special char if we didn't ask for it
    }

    if (special && sp) {
        *special = sp;  // return special char only if requested and provided
        return len;
    }

    if (level) {
        *level = lev;   // return log level
    }

    return len;
}

static void klog_write_to_console(struct console *cons, int start, int count)
{
    // clamp params
    start %= KERNEL_LOG_SIZE;
    count = (count > _klog_count) ? _klog_count : count;

    char * const p_start = &__klog[start];
    char *p = p_start;

    const char *cons_prefix, *cons_suffix;

    int nwritten = 0;
    int nprefix = 0;
    int ntime = 0;

    // TODO: need to handle ring buffer wrap!!
    while (nwritten + nprefix < count) {
        nprefix = klog_parse_prefix(p, &_klog_curr_level, NULL);
        p += nprefix;

        int n = 0;
        for (; (nwritten + nprefix + n < count) &&
               (p + n < __klog + KERNEL_LOG_SIZE); n++) {
            if (klog_parse_prefix(p + n, NULL, NULL)) {
                break;
            }
        }

        if (_klog_curr_level > _klog_level) {
            goto next;
        }

        cons_prefix = _klog_console_level_prefix[_klog_curr_level];
        cons_suffix = _klog_console_level_suffix;

        if (_kprint_time) {
            ntime = 0;
            while (ntime < n) {
                ntime++;
                if (*(p + ntime - 1) == ']') {
                    break;
                }
            }
            cons->write(cons, _klog_console_time_prefix, strlen(_klog_console_time_prefix));
            cons->write(cons, p, ntime);
        }

        int nmod = 0;
        while (ntime + nmod < n) {
            nmod++;
            if (*(p + ntime + nmod) == ' ') {
                nmod = 0;
                break;
            }
            else if (*(p + ntime + nmod) == ':') {
                break;
            }
        }

        if (nmod > 0) {
            cons->write(cons, "\e[33m", 5);
            cons->write(cons, p+ntime, nmod);
        }

        cons->write(cons, cons_prefix, strlen(cons_prefix));
        cons->write(cons, p+ntime+nmod, n-ntime-nmod);
        cons->write(cons, cons_suffix, strlen(cons_suffix));

    next:
        nwritten += n;
        p += nwritten;
    }

    // int end = start + count;
    // if (end < KERNEL_LOG_SIZE) {
    //     cons->write(cons, &__klog[start], count);
    // }
    // else {
    //     cons->write(cons, &__klog[start], KERNEL_LOG_SIZE - start);
    //     cons->write(cons, &__klog[0], end - KERNEL_LOG_SIZE);
    // }
}

static int klog_write_char(char c)
{
    __klog[(_klog_tail + _klog_count) % KERNEL_LOG_SIZE] = c;
    if (_klog_count < KERNEL_LOG_SIZE) {
        _klog_count++;
    }
    else {
        _klog_tail += 1;
        _klog_tail %= KERNEL_LOG_SIZE;
    }

    return 1;
}

int vkprint(const char *fmt, va_list args)
{
    static bool start_new_line = true;
    static bool in_kprint = false;

    char timebuf[32];
    const char *p, *tp;
    struct console *cons;

    char special = 0;   // prefix special char
    int nprefix = 0;    // prefix length
    int nprinted = 0;   // chars printed to log
    int nbufwrit = 0;   // chars written to _kprint_buf
    unsigned int log_ptr = (_klog_tail + _klog_count) % KERNEL_LOG_SIZE;
    unsigned int log_level = DEFAULT_LOG_LEVEL;

    if (in_kprint) {
        const char *bug_msg = KLOG_ERROR "BUG: recent kprint recursion!\n";
        strcpy(_kprint_buf, bug_msg);
        nbufwrit += strlen(bug_msg);
    }
    in_kprint = true;

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
    #error "config: no console enabled for early print!"
  #endif
        early_cons_registered = true;
    }
#endif

    nbufwrit += vsnprintf(_kprint_buf + nbufwrit,
        sizeof(_kprint_buf) - nbufwrit, fmt, args);

    p = _kprint_buf;
    nprefix = klog_parse_prefix(p, &log_level, &special);
    if (nprefix > 0) {
        p += nprefix;
        switch (special) {
            case 'c':   //    LOG_CONT: strip <c> and continue line
                nprefix = 0;
                break;
            case 'd':   // LOG_DEFAULT: strip <d> and print new line
                nprefix = 0;
                __fallthrough;
            default:
                if (!start_new_line) {
                    nprinted += klog_write_char('\n');
                    start_new_line = true;
                }
                break;
        }
    }

    for (; *p; p++) {
        if (start_new_line) {
            start_new_line = false;
            if (nprefix > 0) {
                for (int i = 0; i < nprefix; i++) {
                    nprinted += klog_write_char(_kprint_buf[i]);
                }
            }
            else {
                nprinted += klog_write_char('<');
                nprinted += klog_write_char('0' + log_level);
                nprinted += klog_write_char('>');
            }

            if (_kprint_time) {
                uint64_t ns = get_uptime();
                snprintf(timebuf, sizeof(timebuf),
                    "[%5lu.%06lu] ",
                    (uint32_t) (ns / 1000000000),           // seconds
                    (uint32_t) ((ns % 1000000000) / 1000)); // microseconds
                for (tp = timebuf; *tp; tp++) {
                    nprinted += klog_write_char(*tp);
                }
            }
        }

        nprinted += klog_write_char(*p);
        if (*p == '\n') {
            start_new_line = true;
        }
    }

    cons = g_consoles;
    while (cons) {
        klog_write_to_console(cons, log_ptr, nprinted);
        cons = cons->next;
    }

    in_kprint = false;
    return nprinted;
}

int kprint(const char *fmt, ...)
{
    size_t count;
    va_list args;

    va_start(args, fmt);
    count = vkprint(fmt, args);
    va_end(args);

    return count;
}

void __noreturn panic(const char *fmt, ...)
{
    char buf[BUFSIZ];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    pr_fatal(KLOG_FATAL "panic: %s", buf);

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

bool register_console(struct console *cons)
{
    struct console *curr_cons;
    struct console *prev_cons;
    bool success = true;

    // sanity check
    if (!cons || !cons->name || !cons->device || !cons->init ||
        !cons->write || !cons->read_char) {
        return false;
    }

    // add console to tail
    if (g_consoles == NULL) {
        g_consoles = cons;
    }
    else {
        assert(g_consoles != NULL);
        curr_cons = &g_consoles[0];
        do {
            if (curr_cons == cons) {
                goto registered;
            }
            prev_cons = curr_cons;
            curr_cons = curr_cons->next;
        } while (curr_cons != NULL);
        prev_cons->next = cons;
    }

    // initialize
    success = cons->init(cons);
    cons->next = NULL;

    if (cons->flags & _CONSOLE_FLAG_PRINTBUF) {
        // flush entire log to console
        klog_write_to_console(cons, _klog_tail, _klog_count);
    }

registered:
    return success;
}

bool unregister_console(struct console *cons)
{
    struct console *curr_cons = &g_consoles[0];
    struct console *prev_cons = NULL;

    do {
        if (curr_cons == cons) {
            break;
        }
        prev_cons = curr_cons;
        curr_cons = curr_cons->next;
    } while (curr_cons != NULL);

    if (curr_cons == NULL) {
        return false;   // not found
    }

    if (curr_cons == g_consoles) {
        assert(prev_cons == NULL);
        g_consoles = curr_cons->next;
    }
    else {
        assert(prev_cons != NULL);
        prev_cons->next = curr_cons->next;
    }

    curr_cons->next = NULL;
    return true;
}

#if 0
static void print_log(void)    // TODO: procfs for this
{
    for (int i = 0; i < _log_size; i++) {
        // printf("%c", _kernel_log[(_log_start + i) % _log_size]);
    }
}

static void print_consoles(void)   // TODO: procfs for this
{
    struct console *cons;

    cons = g_consoles;
    while (cons) {
        // printf("%s ", cons->name);
        cons = cons->next;
    }
    // printf("\n");
}
#endif

