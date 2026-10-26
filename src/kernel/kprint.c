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

// This is basically Linux's printk code. Sue me, Linus!

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

// TODO: maybe a timer to print any incomplete log lines;
//      like, if the user calls kprint() without '\n' at the end, and no other
//      kprint calls happen for a long time, make sure that last message shows
//      up within some short amount of time. If the user had intended to use <c>
//      to continue the line, it should've happened long before this timeout

#if CONFIG_KLOG_USE_COLOR
// TODO: make color prefixes configurable
static const char* _klog_level_prefix[MAX_KLOG_LEVEL+1] =
{
/* KLOG_LEVEL_FATAL */ "\e[0;91m",
/* KLOG_LEVEL_ERROR */ "\e[0;31m",
/* KLOG_LEVEL_ALERT */ "\e[0;5;93m\a",
/* KLOG_LEVEL_WARN  */ "\e[0;93m",
/* KLOG_LEVEL_INFO  */ "\e[0;37m",
/* KLOG_LEVEL_DEBUG */ "\e[0;90m",
};
static const char *_klog_level_suffix = ANSI_RESET;
static const char *_klog_timestamp_color = "\e[0;32m";
#endif

static volatile klog_level_t _klog_log_level = CONFIG_KLOG_LEVEL;  // can control w/ SysRq+<0-5>
static klog_level_t _klog_curr_msg_level = CONFIG_KLOG_DEFAULT_MSG_LEVEL;

static struct ring _klog_ring =
    RING_INITIALIZER(__klog, (CONFIG_KLOG_RING_SIZE) << PAGE_SHIFT);

static char _kprint_buf[CONFIG_KPRINT_BUFSIZ] = { };

static int _klog_suspend_count = 0; // suspend() calls without matching resume()
static size_t _klog_flush_tail = 0; // index one past the last flushed '\n'

struct console *g_consoles = NULL;  // linked list of registered consoles

static int klog_ring_write_char(char c);
static int klog_ring_write_buf(const char *buf, size_t count);
static int parse_log_prefix(const char *buf, klog_level_t *level, char *special);
static int klog_ring_parse_prefix(size_t off, klog_level_t *level, char *special);
static int klog_write_line_prefix(void);
static size_t klog_line_begin_index(size_t pos);
static size_t klog_current_line_begin_index(void);
static klog_level_t klog_get_line_level(size_t off);
static size_t klog_emit_timestamp(struct console *cons, size_t offset, size_t nremain);
static void klog_write_to_console(struct console *cons, size_t offset, size_t count);
static void klog_flush_to_consoles(size_t off, size_t count);
static void klog_flush_lines(void);

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#if CONFIG_E9_HACK && CONFIG_E9_HACK_CONSOLE
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
    return __mkdev(0, 0);   // hmm... maybe this is ok...
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

static void register_early_consoles(void)
{
    // suppress writes to consoles until resume_klog() called
    suspend_klog();

#if CONFIG_E9_HACK && CONFIG_E9_HACK_CONSOLE
    static bool e9_console_registered = false;
    if (!e9_console_registered) {
        e9_console_registered = true;
        if (!register_console(&e9_console)) {
            pr_error("failed to register e9_console!\n");
        }
    }
#endif

#if CONFIG_EARLY_PRINT
    static bool early_cons_registered = false;
    if (!early_cons_registered) {
        early_cons_registered = true;
  #if CONFIG_VT_CONSOLE
        extern struct console vt_console;       // see char/terminal.c
        if (!register_console(&vt_console)) {
            pr_error("failed to register vt_console!\n");
        }
  #elif CONFIG_SERIAL_CONSOLE
        extern struct console serial_console;   // see char/serial.c
        if (!register_console(&serial_console)) {
            pr_error("failed to register serial_console!\n");
        }
  #else
    #error "config: no console enabled for early print!"
  #endif
    }
#endif

    // flush all log messages that may have been written during registration
    resume_klog();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static int klog_ring_write_char(char c)
{
    char _c = c;
    if (ring_full(&_klog_ring)) {
        (void) ring_pop_front(&_klog_ring, c, char); // overwrite if full
    }

    return ring_push_back(&_klog_ring, _c, char);
}

static int klog_ring_write_buf(const char *buf, size_t count)
{
    size_t nwritten = 0;
    while (buf && *buf && (count--)) {
        nwritten += klog_ring_write_char(*buf++);
    }

    return nwritten;
}

// parse the <N> prefix at the start of the buffer;
//    returns number of characters consumed if successful, 0 if unsuccessful;
//    if successful, 'level' contains the parsed level, if provided;
//    if 'special' provided and <c>/<d>, 'special' set to 'c' or 'd', 'level'
//    not set
static int parse_log_prefix(const char *buf, klog_level_t *level, char *special)
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
            if (lev > MAX_KLOG_LEVEL) {
                return 0;
            }
            break;
    }
    len = 3;

    if (!special && sp) {
        return 0;       // reject special char if we didn't ask for it
    }
    if (special && sp) {
        *special = sp;
        return len;
    }
    if (level) {
        *level = lev;   // set log level only if valid and special not requested
    }

    return len;
}

// scan up to 3 chars from the ring at 'off' and parse <N> prefix there;
//    returns the number of chars consumed (0 if no valid prefix)
static int klog_ring_parse_prefix(size_t off, klog_level_t *level, char *special)
{
    char buf[3] = { };
    for (int k = 0; k < (int) sizeof(buf); k++) {
        if (!ring_get_at(&_klog_ring, off + k, buf[k], char)) {
            break;
        }
    }

    return parse_log_prefix(buf, level, special);
}

// write the line prefix for the current log level;
//    if timestamps are enabled, the prefix is "<N>[<sec>.<usec>] ",
//    otherwise the prefix is simply "<N>"
static int klog_write_line_prefix(void)
{
    int n = 0;
    n += klog_ring_write_char('<');
    n += klog_ring_write_char('0' + _klog_curr_msg_level);
    n += klog_ring_write_char('>');

#if CONFIG_KLOG_SHOW_TIME
    char timestamp[24];
    uint64_t ns = get_uptime(); // nanoseconds
    int count = snprintf(timestamp, sizeof(timestamp), "[%5lu.%06lu] ",
        (uint32_t) (ns / 1000000000),           // seconds
        (uint32_t) ((ns % 1000000000) / 1000)); // microseconds
    n += klog_ring_write_buf(timestamp, count);
#endif

    return n;
}

// find the log level of the log line that passes through the given ring offset
static klog_level_t klog_get_line_level(size_t off)
{
    klog_level_t level = CONFIG_KLOG_DEFAULT_MSG_LEVEL;
    klog_ring_parse_prefix(klog_line_begin_index(off), &level, NULL);

    return level;
}

// scan backwards in the ring from 'pos' to find the start of the last log line;
//    the start of the log line is the index one past the previous '\n',
//    or the ring front
static size_t klog_line_begin_index(size_t pos)
{
    char c;
    while (pos > 0) {
        if (!ring_get_at(&_klog_ring, pos - 1, c, char) || c == '\n') {
            break;
        }
        pos--;
    }

    return pos;
}

// returns the index of the start of the current (incomplete) line
static size_t klog_current_line_begin_index(void)
{
    return klog_line_begin_index(ring_count(&_klog_ring));
}

// flush all complete log lines from the ring buffer to the consoles;
//    a log line is said to be "complete" when it begins with a log prefix '<N>'
//    and end with a newline '\n'
static void klog_flush_lines(void)
{
    if (_klog_suspend_count != 0) {
        return; // ignore flush if suspended
    }
    size_t end_index = klog_current_line_begin_index();
    if (end_index > _klog_flush_tail) {
        klog_flush_to_consoles(_klog_flush_tail, end_index - _klog_flush_tail);
        _klog_flush_tail = end_index;
    }
}

// flush 'count' characters of buffered log text to all consoles starting from
// offset 'off'
static void klog_flush_to_consoles(size_t off, size_t count)
{
    struct console *cons = g_consoles;
    while (cons) {
        if (cons->flags & _CONSOLE_FLAG_KLOG) {
            klog_write_to_console(cons, off, count);
        }
        cons = cons->next;
    }
}

#if CONFIG_KLOG_SHOW_TIME
// prints a timestamp of the form "[<sec>.<usec>]" to the given console if
// present in the ring at [offset, offset+count);
//    if CONFIG_KLOG_USE_COLOR is set, the timestamp will be emitted with
//    color using ANSI escape sequences
static size_t klog_emit_timestamp(struct console *cons, size_t offset, size_t count)
{
    char timestamp[24];
    char c = '\0';

    size_t n = 0;
    while (n < count && n < sizeof(timestamp)) {
        if (!ring_get_at(&_klog_ring, offset + n, c, char) || c == '\n') {
            return 0;
        }
        timestamp[n++] = c;
        if (c == ']') {
            break;
        }
    }
    if (timestamp[0] != '[' || c != ']' || n < 5) {
        return 0;
    }

#if CONFIG_KLOG_USE_COLOR
    cons->write(cons, _klog_timestamp_color, strlen(_klog_timestamp_color));
#endif
    cons->write(cons, timestamp, n);
#if CONFIG_KLOG_USE_COLOR
    cons->write(cons, ANSI_RESET, strlen(ANSI_RESET));
#endif
    return n;
}
#endif

// prints the text present in the ring at [offset, offset+count) to the
// specified console;
//    if CONFIG_KLOG_USE_COLOR is set, the message is printed with color
//    according to it's log level, the timestamp (if present) will also be
//    printed with color;
//    the message printed may come from anywhere in the string -- it doesn't
//    need to be a complete line, though ideally complete lines are printed as
//    it makes for proper prefix and timestamp detection (for injecting color)
static void klog_write_to_console(struct console *cons, size_t offset, size_t count)
{
    // clamp params to the valid relative range
    size_t max_offset = ring_count(&_klog_ring);
    if (offset > max_offset) {
        offset = max_offset;
    }
    if (offset + count > max_offset) {
        count = max_offset - offset;
    }

    bool s_start_new_line = (offset == 0);
    if (!s_start_new_line) {
        char prev;
        if (ring_get_at(&_klog_ring, offset - 1, prev, char) && prev == '\n') {
            s_start_new_line = true;
        }
    }

    size_t nremain = count;
    klog_level_t msg_level = klog_get_line_level(offset);
    while (nremain > 0) {
        // consume an <N> prefix at the start of a line segment, if present
        int nprefix = klog_ring_parse_prefix(offset, &msg_level, NULL);
        offset += nprefix;
        nremain -= nprefix;

        bool emit = (msg_level <= _klog_log_level);

        // emit timestamp and log level formatting prefix
        if (s_start_new_line) {
            s_start_new_line = false;
            if (emit) {
#if CONFIG_KLOG_SHOW_TIME
                // need a separate function for this so we can surround the
                // timestamp with color escape sequences, provided that
                // CONFIG_KLOG_USE_COLOR is set
                size_t ntimestamp = klog_emit_timestamp(cons, offset, nremain);
                offset += ntimestamp;
                nremain -= ntimestamp;
#endif
#if CONFIG_KLOG_USE_COLOR
                const char *prefix = _klog_level_prefix[msg_level];
                cons->write(cons, prefix, strlen(prefix));
#endif
            }
        }

        // write the message body
        while (nremain > 0) {
            char c;
            if (!ring_get_at(&_klog_ring, offset, c, char)) {
                break;
            }

            // stop if <N> found at start of next line
            if (s_start_new_line && klog_ring_parse_prefix(offset, NULL, NULL) > 0) {
                break;
            }

            if (c == '\n') {
                s_start_new_line = true;
#if CONFIG_KLOG_USE_COLOR
                if (emit) {
                    cons->write(cons, _klog_level_suffix, strlen(_klog_level_suffix));
                }
#endif
            }

            if (emit) {
                cons->write(cons, &c, 1);
            }

            offset += 1;
            nremain -= 1;
        }
    }
}

// ----------------------------------------------------------------------------

void suspend_klog(void)
{
    _klog_suspend_count++;
}

void resume_klog(void)
{
    if (_klog_suspend_count == 0) {
        return;
    }

    _klog_suspend_count--;
    if (_klog_suspend_count == 0) {
        klog_flush_lines();
    }
}

klog_level_t get_klog_level(void)
{
    return _klog_log_level;
}

void set_klog_level(klog_level_t level)
{
    if (level <= MAX_KLOG_LEVEL) {
        _klog_log_level = level;
    }
}

int vkprint(const char *fmt, va_list args)
{
    static bool s_start_new_line = true;
    static bool s_in_kprint = false;

    char special = 0;       // prefix special char (<c>, <d>, etc.)
    int nprefix = 0;        // prefix length
    int nprinted = 0;       // num chars printed to log
    int nbufwrit = 0;       // num chars written to _kprint_buf
    bool recursed = false;  // oops! we printed while printing...
    const char *p;

#if 0
    static bool s_test_kprint = true;
    if (s_test_kprint) {
        s_test_kprint = false;
        kprint("kprint() test:\n");
        pr_fatal("fatal message\n");
        pr_error("error message\n");
        pr_alert("alert message\n");
        pr_warn("warn message\n");
        pr_info("info message\n");
        pr_debug("debug message\n");
        pr_info("continued"); pr_cont(" message\n");
        pr_error("continued"); pr_cont(" error message\n");
        pr_info("              leading space\n");
        pr_info("trailing space             \n");
        pr_info("\nleading newline");
        pr_info("no newline");
        pr_info("should be on its own line\n");
        pr_info("");    // should not emit empty/blank lines
        pr_info("    ");
        pr_info("\n");
        kprint("continue"); kprint(" kprint on same line\n");
        kprint("[123.456] fake timestamp\n");
        kprint("<5>manual log level\n");
        kprint("<5>[123.456] manual log level with fake timestamp\n");
        kprint("stray<3> log level\n");
        kprint("embedded\nnewline\n");
        kprint("embedded\n<5>newline with embedded log level\n");
        kprint("test complete.\n");
    }
#endif

    // check for and warn about kprint recursion
    if (s_in_kprint) {
        recursed = true;
        nbufwrit += snprintf(_kprint_buf, sizeof(_kprint_buf),
            _KLOG_ERROR("BUG: kprint() recursion detected!\n"));
    }
    s_in_kprint = true;

    // parse printf format string
    nbufwrit += vsnprintf(_kprint_buf + nbufwrit,
        sizeof(_kprint_buf) - nbufwrit, fmt, args);

    // capture the ring position before storing, so the flush tail can be
    // rebased for any entries evicted during this call
    size_t ring_before = ring_count(&_klog_ring);

    // parse <N> prefix, if present
    p = _kprint_buf;
    nprefix = parse_log_prefix(p, &_klog_curr_msg_level, &special);
    if (nprefix == 0) {
        _klog_curr_msg_level = CONFIG_KLOG_DEFAULT_MSG_LEVEL;
    }
    else {
        p += nprefix;
        switch (special) {
            case 'c':       // continue: strip <c> and continue line
                nprefix = 0;
                break;
            case 'd':       // default: strip <d>, set level, and print new line
                nprefix = 0;
                _klog_curr_msg_level = CONFIG_KLOG_DEFAULT_MSG_LEVEL;
                __fallthrough;
            default:
                if (!s_start_new_line) {
                    nprinted += klog_ring_write_char('\n');
                    s_start_new_line = true;
                }
                break;
        }
    }

    // write the log message to the ring; lines are stored in a canonical
    // format: "<N>[sec.usec] text" -- an explicit <N> prefix is honored only
    // when it appears at a line start, anywhere else it is literal text
    for (; *p; p++) {
        if (s_start_new_line) {
            // at a line start, an explicit <N> prefix is consumed and
            // becomes this line's log level until the next line start
            char line_special = 0;
            int nlprefix = parse_log_prefix(p, &_klog_curr_msg_level, &line_special);
            if (nlprefix > 0) {
                p += nlprefix;
                if (line_special == 'd') {
                    _klog_curr_msg_level = CONFIG_KLOG_DEFAULT_MSG_LEVEL;
                }
            }

            const char *tmp = p;
            while (isspace(*tmp) && *tmp != '\n') {
                tmp++;
            }
            if (*tmp == '\0') {
                break;      // skip blank lines entirely
            }
            if (*tmp == '\n') {
                p = tmp;    // skip to the newline,
                continue;   // then consume that newline
            }

            // store the canonical line prefix: log level, then timestamp
            s_start_new_line = false;
            nprinted += klog_write_line_prefix();
        }

        // emit consumed character
        nprinted += klog_ring_write_char(*p);
        if (*p == '\n') {
            s_start_new_line = true;
        }
    }

    // rebase the flush tail for entries evicted while storing this
    // message (storing into a full ring evicts the oldest entry)
    size_t evicted = ring_before + nprinted - ring_count(&_klog_ring);
    if (evicted > 0) {
        _klog_flush_tail =
            (evicted >= _klog_flush_tail) ? 0 : _klog_flush_tail - evicted;
    }

    // flush complete lines (a partial line stays buffered until closed)
    klog_flush_lines();

    // -----------------------------------------------------------------------

    // register consoles lazily on first print; skip if recursed because
    // lazy console inits can be spew'y
    if (!recursed) {
        register_early_consoles();
    }

    s_in_kprint = false;
    return nbufwrit;
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

// ----------------------------------------------------------------------------

bool register_console(struct console *cons)
{
    struct console *curr_cons;
    struct console *prev_cons;

    // sanity check
    if (!cons || !cons->name || !cons->device || !cons->init ||
        !cons->write || !cons->read_char) {
        return false;
    }

    // add console to tail of list
    if (g_consoles == NULL) {
        g_consoles = cons;
    }
    else {
        assert(g_consoles != NULL);
        curr_cons = &g_consoles[0];
        do {
            if (curr_cons == cons) {
                return true;    // already registered
            }
            prev_cons = curr_cons;
            curr_cons = curr_cons->next;
        } while (curr_cons != NULL);
        prev_cons->next = cons;
    }
    cons->next = NULL;

    // initialize console
    if (!cons->init(cons)) {
        unregister_console(cons);   // if failed, remove from list
        return false;
    }

    // write entire log buffer to the newly-registered console,
    // stopping at the current in-progress line
    if (cons->flags & _CONSOLE_FLAG_KLOG) {
        klog_write_to_console(cons, 0, klog_current_line_begin_index());
    }

    return true;
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
