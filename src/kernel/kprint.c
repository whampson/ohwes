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
#include <ring.h>
#include <i386/io.h>
#include <kernel/console.h>
#include <kernel/kernel.h>
#include <kernel/kprint.h>
#include <kernel/irq.h>
#include <kernel/mm.h>
#include <kernel/terminal.h>

#define LOG_LEVEL_MAX  5    // <5> = LOG_DEBUG

// TODO: configurable
const char* _klog_console_level_prefix[LOG_LEVEL_MAX+1] =
{
/* KLOG_FATAL */ "\e[91m",
/* KLOG_ERROR */ "\e[91m",
/* KLOG_ALERT */ "\e[93m",
/* KLOG_WARN  */ "\e[93m",
/* KLOG_INFO  */ "\e[0;37m",
/* KLOG_DEBUG */ "\e[0;37m",
};
const char *_klog_console_level_suffix = "\e[0;39m";

static unsigned int _klog_set_level = DEFAULT_LOG_LEVEL; // current log level; higher is less important // TODO: control this somehow
static unsigned int _klog_curr_msg_level = DEFAULT_LOG_LEVEL;

struct ring klog_ring = RING_INIT(__klog, KERNEL_LOG_SIZE);

static char _kprint_buf[BUFSIZ] = { };

struct console *g_consoles = NULL;  // linked list

// ----------------------------------------------------------------------------

#if E9_HACK && ENABLE_E9HACK_CONSOLE
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
    .flags = _CONSOLE_FLAG_KLOG,
    .device = e9_console_device,
    .init = e9_console_setup,
    .write = e9_console_write,
    .read_char = e9_console_read_char,
};
#endif

// ----------------------------------------------------------------------------

static int klog_write_char(char c)
{
    if (ring_full(&klog_ring)) {
        char tmp;
        (void) ring_pop_front(&klog_ring, tmp, char);
        (void) tmp; // this is annoying...
    }

    return ring_push_back(&klog_ring, c, char);
}

static int klog_write_buf(const char *buf, size_t count)
{
    size_t nwritten = 0;
    while (buf && *buf && (count--)) {
        nwritten += klog_write_char(*buf++);
    }

    return nwritten;
}

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

static void klog_write_to_console(struct console *cons, int offset, int count)
{
    // clamp params
    offset %= ring_capacity(&klog_ring);
    count = (count > ring_count(&klog_ring)) ? ring_count(&klog_ring) : count;

    unsigned int msg_level = _klog_curr_msg_level;

    char prefix_buf[3];
    int nprefix;
    int nremain;

    // TODO: ideally I'd like to buffer 'til I see a prefix or reach 'count',
    // then print the buffer and repeat 'til we hit the end... but this will do
    // for now

    nremain = count;
    while (nremain > 0) {
        prefix_buf[0] = '\0';
        for (int k = 0; k < sizeof(prefix_buf); k++) {
            if (!ring_get_at(&klog_ring, offset+k, prefix_buf[k], char)) {
                prefix_buf[k] = '\0';
                break;
            }
        }
        nprefix = klog_parse_prefix(prefix_buf, &msg_level, NULL);
        offset += nprefix;
        nremain -= nprefix;

        int idx = 0;
        do {
            if (nprefix == 0) {
                if (msg_level <= _klog_curr_msg_level && msg_level <= _klog_set_level) {
                    cons->write(cons, &prefix_buf[idx++], 1);
                }
                offset += 1;
                nremain -= 1;
            }
        } while (nremain < sizeof(prefix_buf));
    }
}

int vkprint(const char *fmt, va_list args)
{
    static bool start_new_line = true;
    static bool in_kprint = false;

    char special = 0;   // prefix special char (<c>, <d>, etc.)
    int nprefix = 0;    // prefix length
    int nprinted = 0;   // num chars printed to log
    int nbufwrit = 0;   // num chars written to _kprint_buf
    unsigned int log_ptr = ring_count(&klog_ring);

    const char *p;
    struct console *cons;

    // check for and warn about kprint recursion
    if (in_kprint) {
        const char *bug_msg = KLOG_ERROR "BUG: kprint recursion detected!\n";
        strcpy(_kprint_buf, bug_msg);
        nbufwrit += strlen(bug_msg);
    }
    in_kprint = true;

    // parse printf format string
    nbufwrit += vsnprintf(_kprint_buf + nbufwrit,
        sizeof(_kprint_buf) - nbufwrit, fmt, args);

    // parse <N> prefix
    p = _kprint_buf;
    nprefix = klog_parse_prefix(p, &_klog_curr_msg_level, &special);
    if (nprefix > 0) {
        p += nprefix;
        switch (special) {
            case 'c':   //    LOG_CONT: strip <c> and continue line
                nprefix = 0;
                break;
            case 'd':   // LOG_DEFAULT: strip <d> and print new line
                nprefix = 0;
                _klog_curr_msg_level = DEFAULT_LOG_LEVEL;
                __fallthrough;
            default:
                if (!start_new_line) {
                    nprinted += klog_write_char('\n');
                    start_new_line = true;
                }
                break;
        }
    }

    // count leading spaces
    int nspace = 0, nlinefeed = 0;
    for (; nspace < nbufwrit - nprefix; nspace++) {
        if (!isspace(*(p + nspace))) {
            break;
        }
        if (*(p + nspace) == '\n') {
            nlinefeed++;
        }
    }

    // don't allow empty/blank log lines
    if (nspace == nbufwrit - nprefix) {
        goto kprint_done;
    }

    // skip past leading newlines; leave one if start_new_line==false
    if (nlinefeed) {
        p += nlinefeed - (!start_new_line);
    }

    // write the log line to the buffer
    for (; *p; p++) {
        if (start_new_line) {
            start_new_line = false;
            if (nprefix > 0) {
                nprinted += klog_write_buf(_kprint_buf, nprefix);
            }
            else {
                nprinted += klog_write_char('<');
                nprinted += klog_write_char('0' + _klog_curr_msg_level);
                nprinted += klog_write_char('>');
            }

        // #if KPRINT_COLOR
            const char *ansi_suffix = _klog_console_level_suffix;
            nprinted += klog_write_buf(ansi_suffix, strlen(ansi_suffix));   // TODO: move formatting to log flush/dump
        // #endif

        #if KPRINT_TIME
            char timebuf[64];
            uint64_t ns = get_uptime();
            snprintf(timebuf, sizeof(timebuf),
            #if KPRINT_COLOR
                ANSI_UNBOLD ANSI_GREEN
            #endif
                "[%5lu.%06lu] ",         // TODO: move color formatting to log flush/dump
                (uint32_t) (ns / 1000000000),           // seconds
                (uint32_t) ((ns % 1000000000) / 1000)); // microseconds
            nprinted += klog_write_buf(timebuf, sizeof(timebuf));
        #endif

        #if KPRINT_COLOR
            const char *ansi_prefix = _klog_console_level_prefix[_klog_curr_msg_level];
            nprinted += klog_write_buf(ansi_prefix, strlen(ansi_prefix));   // TODO: move formatting to log flush/dump
        #endif
        }

        nprinted += klog_write_char(*p);
        if (*p == '\n') {
            start_new_line = true;
        }
    }

    // write log message to consoles
    cons = g_consoles;
    while (cons) {
        if (cons->flags & _CONSOLE_FLAG_KLOG) {
            klog_write_to_console(cons, log_ptr, nprinted);
        }
        cons = cons->next;
    }

    // deal with early console registrations;
    //   register_console will dump current klog if _CONSOLE_FLAG_KLOG set
#if E9_HACK && ENABLE_E9HACK_CONSOLE
    static bool e9_console_registered = false;
    if (!e9_console_registered) {
        if (!register_console(&e9_console)) {
            panic("failed to register e9_console!\n");
        }
        e9_console_registered = true;
    }
#endif

#if EARLY_PRINT
    static bool early_cons_registered = false;
    if (!early_cons_registered) {
  #if ENABLE_VT_CONSOLE
        extern struct console vt_console;       // see char/terminal.c
        if (!register_console(&vt_console)) {
            panic("failed to register vt_console!\n");
        }
  #elif ENABLE_SERIAL_CONSOLE
        extern struct console serial_console;   // see char/serial.c
        if (!register_console(&serial_console)) {
            panic("failed to register serial_console!\n");
        }
  #else
    #error "config: no console enabled for early print!"
  #endif
        early_cons_registered = true;
    }
#endif

kprint_done:
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

    if (cons->flags & _CONSOLE_FLAG_KLOG) {
        // flush entire log to console
        klog_write_to_console(cons, 0, ring_count(&klog_ring));
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

