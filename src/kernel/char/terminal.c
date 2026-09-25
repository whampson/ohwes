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
 *         File: kernel/char/terminal.c
 *      Created: March 26, 2023
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <ctype.h>
#include <errno.h>
#include <i386/bitops.h>
#include <i386/boot.h>
#include <i386/cpu.h>
#include <i386/interrupt.h>
#include <i386/io.h>
#include <i386/paging.h>
#include <i386/x86.h>
#include <kernel/kernel.h>
#include <kernel/kprint.h>
#include <kernel/char.h>
#include <kernel/console.h>
#include <kernel/ioctls.h>
#include <kernel/irq.h>
#include <kernel/mm.h>
#include <kernel/tty.h>
#include <kernel/terminal.h>
#include <kernel/termios.h>
#include <kernel/vga.h>

#define VGA_FB_SIZE_PAGES   8                   // B8000-BFFFF
#define VGA_FB_SIZE         (VGA_FB_SIZE_PAGES << PAGE_SHIFT)
#define VGA_FB_WORDS        (VGA_FB_SIZE >> 1)  // 2 bytes per char

#define FAST_SCROLL         1   // uses VGA hardware regs to control scrolling
#define VSYNC               1   // sync large screen updates to vertical retrace
#define VSYNC_SCROLL        0   // slows scrolling, but no tearing!
// TODO: smooth scroll? :D

#define __mkwinsize(rows, cols) \
    (struct winsize) { (rows), (cols), 0, 0 }

// TODO: rename 'vgaterm.c'
// TODO: screen blanking

static void initialize_terminal(int num, struct terminal *term);
static void terminal_defaults(struct terminal *term);
static int print_to_terminal(struct terminal *term, const char *str);
static int write_to_terminal(struct terminal *term, const char *buf, size_t count);
static uint16_t xy2pos(const struct terminal *term, uint16_t x, uint16_t y);
static void pos2xy(struct terminal *term, uint16_t pos);
static bool is_current_terminal(struct terminal *term);

static int s_currterm = DEFAULT_VT;
static struct terminal s_terms[NR_TERMINAL];

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// TTY device implementation

static int s_driver_refcnt;
static struct tty *s_term_ttys[NR_TERMINAL];
static struct termios *s_term_termios[NR_TERMINAL];

static int terminal_tty_open(struct tty *);
static void terminal_tty_close(struct tty *);
static int terminal_tty_ioctl(struct tty *, int op, void *arg);
static int terminal_tty_write(struct tty *, const char *buf, size_t count);
static void terminal_tty_write_char(struct tty *, char c);
static size_t terminal_tty_write_room(struct tty *);

static int terminal_tiocgwinsz(struct terminal *term, struct winsize *winsize);
static int terminal_tiocswinsz(struct terminal *term, const struct winsize *winsize);

static struct tty_driver terminal_driver = {
    .major = TTY_MAJOR,
    .minor_start = TTY_MIN,
    .device_count = NR_TERMINAL,
    .name = "tty",
    .refcount = &s_driver_refcnt,
    .tty_table = s_term_ttys,
    .termios = s_term_termios,
    .default_termios = TTY_STD_TERMIOS,
    .open = terminal_tty_open,
    .close = terminal_tty_close,
    .ioctl = terminal_tty_ioctl,
    .write = terminal_tty_write,
    ./* in the */write_room/* with black curtains*/ = terminal_tty_write_room,
    .flush = NULL,
    .clear = NULL,
    .unthrottle = NULL,
    .throttle = NULL,
    .stop= NULL,
    .start = NULL,
    .hangup = NULL,
};

static int tty_get_terminal(struct tty *tty, struct terminal **term)
{
    if (!tty || !term) {
        return -EINVAL;
    }

    if (_DEV_MAJ(tty->device) != TTY_MAJOR) {
        return -ENODEV; // not a TTY device
    }

    int index = _DEV_MIN(tty->device);
    if (index < TTY_MIN || index > TTY_MAX) {
        return -ENXIO;  // TTY device is not a virtual terminal
    }

    *term = get_terminal(index);
    return 0;
}

static int terminal_tty_open(struct tty *tty)
{
    struct terminal *term;

    int ret = tty_get_terminal(tty, &term);
    if (ret < 0) {
        return ret;
    }

    if (term->tty) {
        goto open_done;
    }

    term->tty = tty;
    tty->winsz = __mkwinsize(term->rows, term->cols);
    // TODO: other startup/connect operations

open_done:
    term->refcount++;
    return 0;
}

static void terminal_tty_close(struct tty *tty)
{
    struct terminal *term;

    int ret = tty_get_terminal(tty, &term);
    if (ret < 0) {
        return;
    }

    if (--term->refcount < 0) {
        term->refcount = 0;
    }
    if (term->refcount) {
        goto close_done;
    }

    term->tty = NULL;
    tty->winsz = __mkwinsize(0, 0);
    // TODO: other shutdown operations

close_done:
    return;
}

static int terminal_tty_write(struct tty *tty, const char *buf, size_t count)
{
    struct terminal *term;

    if (!buf) {
        return -EINVAL;
    }

    int ret = tty_get_terminal(tty, &term);
    if (ret < 0) {
        return ret;
    }

    ret = write_to_terminal(term, buf, count);
    if (tty->driver.flush) {
        tty->driver.flush(tty);
    }

    return ret;
}

static int terminal_tty_ioctl(struct tty *tty, int op, void *arg)
{
    return -ENOTTY;
}

static size_t terminal_tty_write_room(struct tty *tty)
{
    // we can write the frame buffer forever...
    // return something sufficiently large to satisfy ldisc logic
    return (tty->stopped) ? 0 : 4096;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// console implementation

#if ENABLE_VT_CONSOLE

static dev_t vt_console_device(struct console *cons)
{
    return __mkttydev((cons->number) ? cons->number : s_currterm);
}

__init void init_early_terminal(int num, struct terminal *term)
{
    struct vga_fb_info fb_info;
    initialize_terminal(num, term);
    s_currterm = num;

    // inherit virtual terminal properties from VGA,
    // VGA likely hasn't been set-up yet, so we can't assume the frame buffer
    // assignment made by initialize_terminal() is correct;
    // grab the actual frame buffer address currently in use
    vga_get_fb_info(&fb_info);
    term->framebuf = KERNEL_ADDR(fb_info.base_physical);
    term->backbuf = fb_info.base_physical;
    pos2xy(term, vga_get_cursor_pos());
}

static __init bool vt_console_setup(struct console *cons)
{
    struct terminal *term = get_terminal(cons->number);
    if (!term->initialized) {
        int num = _DEV_MIN(cons->device(cons));
        init_early_terminal(num, term);
    }

    return true;
}

static ssize_t vt_console_write(struct console *cons, const char *buf, size_t count)
{
    struct terminal *term;
    const char *p;

    term = get_terminal(cons->number);

    p = buf;
    while ((p - buf) < count) {
        if (*p == '\n') {   // TODO: some kind of termios?
            __terminal_putc(term, '\r');
        }
        __terminal_putc(term, *p++);
    }

    return (p - buf);
}

static int vt_console_getc(struct console *cons)
{
    char c;
    struct tty *tty;

    if (!kb_avail()) {
        return -EAGAIN;
    }

    tty = get_terminal(0)->tty;

    while (true) {
        if (!kb_avail()) {
            return -EAGAIN;
        }
        c = __n_tty_getc(tty);
        if (c != -EAGAIN) {
            break;
        }
        // TODO: scheduler yield instead of busy loop
    }

    return c;
}

struct console vt_console =
{
    .name = "tty",
    .number = VT_CONSOLE_NUM,
    .flags = _CONSOLE_FLAG_KLOG,
    .device = vt_console_device,
    .init = vt_console_setup,
    .write = vt_console_write,
    .read_char = vt_console_getc
};

#endif

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// virtual terminal implementation

enum terminal_state {
    S_NORM,
    S_ESC,
    S_CSI
};

enum erase_mode {
    ERASE_DOWN,
    ERASE_UP,
    ERASE_ALL
};

// terminal state
static void reset_terminal(struct terminal *term);       // ESC c
static void save_terminal(struct terminal *term);        // ESC 7
static void restore_terminal(struct terminal *term);     // ESC 8
static void save_cursor(struct terminal *term);          // ESC [s
static void restore_cursor(struct terminal *term);       // ESC [u

// character handling
static void esc(struct terminal *term, char c);          // ^[ (ESC)
static void csi(struct terminal *term, char c);          // ESC [
static void csi_m(struct terminal *term, char p);        // ESC [<params>m
static void backspace(struct terminal *term);            // ^H
static void tab(struct terminal *term);                  // ^I
static void line_feed(struct terminal *term);            // ^J
static void reverse_linefeed(struct terminal *term);     // ESC M
static void carriage_return(struct terminal *term);      // ^M
static void scroll(struct terminal *term, int n);        // ESC [<n>S / ESC [<n>T
static void erase(struct terminal *term, int mode);      // ESC [<n>J
static void erase_line(struct terminal *term, int mode); // ESC [<n>K
static void cursor_up(struct terminal *term, int n);     // ESC [<n>A
static void cursor_down(struct terminal *term, int n);   // ESC [<n>B
static void cursor_right(struct terminal *term, int n);  // ESC [<n>C
static void cursor_left(struct terminal *term, int n);   // ESC [<n>D

// VGA features
static void apply_blink_state(const struct terminal *term);     // ESC 3 / ESC 4
static void apply_cursor_enabled(const struct terminal *term);  // ESC 5 / ESC 6
static void apply_cursor_pos(const struct terminal *term);      // ESC [ <n>;<m>H
static void apply_cursor_shape(const struct terminal *term);
static void apply_cursor_state(const struct terminal *term);
static void apply_vga_state(const struct terminal *term);

// frame buffer
static void set_fb_char(struct terminal *term, uint16_t pos, char c);
static void set_fb_attr(struct terminal *term, uint16_t pos, struct _char_attr attr);
static void map_terminal_fb(struct terminal *term, uintptr_t phys);

// ----------------------------------------------------------------------------
// initialization

__init void init_terminal_driver(void)
{
    struct vga_fb_info fb_info;

    // set frame buffer physical address
    vga_set_fb(VGA_MEMORY_32K_HI);  // 0xB8000 - 0xBFFFF
    vga_get_fb_info(&fb_info);

    pr_debug("vga: frame buffer %u pages %p-%p\n",
        fb_info.size_pages, KERNEL_ADDR(fb_info.base_physical),
        KERNEL_ADDR(fb_info.base_physical + (fb_info.size_pages << PAGE_SHIFT)-1));

    // get the keyboard working
    extern __init void init_kb(void);
    init_kb();

    // initialize each terminal
    for (int i = 1; i <= NR_TERMINAL; i++) {
        struct terminal *term = get_terminal(i);
        if (!term->initialized) {
            initialize_terminal(i, term);
        }

        // backbuffer allocation
        void *backbuf = alloc_pages(MEM_ZERO, get_order(VGA_FB_SIZE));
        if (!backbuf) {
            panic("unable to allocate frame buffer for terminal %d!", i);
        }
        term->framebuf = backbuf;
        term->backbuf = PHYSICAL_ADDR(backbuf);

        if (term->number == s_currterm) {
            // preserve boot output by copying VGA into back buffer,
            // then map the default terminal's frame buffer into VGA memory
            memcpy(term->framebuf, KERNEL_ADDR(fb_info.base_physical), VGA_FB_SIZE);
            map_terminal_fb(term, fb_info.base_physical);
            pos2xy(term, vga_get_cursor_pos());
        }
        else {
            char buf[32];
            snprintf(buf, sizeof(buf), "\e[2J\e[Htty%d\r\n", i);
            print_to_terminal(term, buf);
        }

        // enable blink, show cursor
        print_to_terminal(term, "\e4\e6");
    }

    // register the terminal TTY driver, needed for terminal switch
    if (tty_register_driver(&terminal_driver)) {
        panic("unable to register terminal driver!");
    }

#if ENABLE_VT_CONSOLE
    // register the virtual terminal console
    register_console(&vt_console);
#endif

    // switch to configured default terminal
    if (s_currterm != DEFAULT_VT) {
        switch_terminal(DEFAULT_VT);
    }
}

static void initialize_terminal(int num, struct terminal *term)
{
    if (term->initialized) {
        return;
    }

    if (num == 0) {
        num = s_currterm;
    }

    terminal_defaults(term);
    term->number = num;
    term->framebuf = NULL;
    term->backbuf = 0;
    clear_bit(&term->printing, 0);
    term->initialized = true;
}

// ----------------------------------------------------------------------------
// public functions

void terminal_defaults(struct terminal *term)
{
    // TODO: why not memset here??

    term->state = S_NORM;
    term->cols = vga_get_cols();
    term->rows = vga_get_rows();
    term->origin = 0;
    for (int i = 0; i < MAX_TABSTOP; i++) {
        term->tabstops[i] = (((i + 1) % TABSTOP_WIDTH) == 0);
    }
    memset(term->csiparam, -1, sizeof(term->csiparam));
    term->paramidx = 0;
    term->blink_on = false;
    term->line_wrap_pending = false;
    term->attr._value = 0;
    term->attr.bg = VGA_BLACK;
    term->attr.fg = VGA_WHITE;
    term->cursor.x = 0;
    term->cursor.y = 0;
    term->cursor.shape = vga_get_cursor_shape();    // TODO: default shape
    term->cursor.hidden = false;
    term->csi_defaults.attr = term->attr;
    term->csi_defaults.cursor = term->cursor;
    save_terminal(term);
}

int switch_terminal(int num)
{
    if (num <= 0 || num > NR_TERMINAL) {
        return -EINVAL;
    }

    struct terminal *curr = get_terminal(0);
    struct terminal *next = get_terminal(num);

    if (curr == next) {
        return 0;
    }
    if (!next->initialized) {
        return -EIO;
    }

    uint32_t flags;
    cli_save(flags);

    struct vga_fb_info fb_info;
    vga_get_fb_info(&fb_info);

    map_terminal_fb(curr, curr->backbuf);
    memcpy(curr->framebuf, KERNEL_ADDR(fb_info.base_physical), VGA_FB_SIZE);
    apply_vga_state(next);                                                      // will vsync
    memcpy(KERNEL_ADDR(fb_info.base_physical), next->framebuf, VGA_FB_SIZE);    // updates the display
    map_terminal_fb(next, fb_info.base_physical);

    next->kb_state._modkeys = curr->kb_state._modkeys;  // preserve current held-down key state
    ps2kb_apply_state(&next->kb_state.hw_state);

    s_currterm = next->number;  // make it official

    restore_flags(flags);
    return 0;
}

static bool is_current_terminal(struct terminal *term)
{
    return term->number == s_currterm;
}

struct terminal * get_terminal(int num)
{
    if (num < 0 || num > NR_TERMINAL) {
        panic("attempt to get nonexistant terminal %d!", num);
    }
    if (num == 0) {
        num = s_currterm;
    }
    assert(num > 0);

    struct terminal *term = &s_terms[num - 1];
    if (term->initialized) {
        assert(term->number == num);
    }

    return term;
}

int print_to_terminal(struct terminal *term, const char *buf)
{
    if (!term || !buf) {
        return -EINVAL;
    }
    return write_to_terminal(term, buf, strnlen(buf, MAX_PRINTBUF));
}

int write_to_terminal(struct terminal *term, const char *buf, size_t count)
{
    const char *p;

    if (!term || !buf) {
        return -EINVAL;
    }

    p = buf;
    while (p < buf + count) {
#if E9_HACK && ENABLE_E9HACK_PRINTF
    if (term == get_terminal(0)) {
        outb(0xE9, *p);
    }
#endif
        __terminal_putc(term, *p++);
    }

    return count;
}

void __terminal_putc(struct terminal *term, char c)
{
    bool update_char = false;
    bool update_attr = false;
    bool update_cursor_pos = true;
    uint16_t char_pos;

    // prevent reentrancy from interrupt/DPC context
    // to avoid mucking with terminal state
    if (test_and_set_bit(&term->printing, 0)) {
        return; // TODO: do we just drop the char?
    }

    // TODO: ioctl to prevent processing of control characters
    // so we can write to the VGA directly

    // handle escape sequences if not a control character
    if (!iscntrl(c)) {
        switch (term->state) {
            case S_ESC:
                esc(term, c);
                goto write_vga;
            case S_CSI:
                csi(term, c);
                goto write_vga;
            case S_NORM:
                break;
            default:
                term->state = S_NORM;
                break;
        }
    }

    // control characters
    switch (c) {
        case '\a':      // ^G - BEL - beep!
            beep(BELL_FREQ, BELL_TIME, false);         // TODO: ioctl to control beep tone/time
            break;
        case '\b':      // ^H - BS - backspace
            backspace(term);
            break;
        case '\t':      // ^I - HT - horizontal tab
            tab(term);
            break;
        case '\n':      // ^J - LF - line feed
            __fallthrough;
        case '\v':      // ^K - VT - vertical tab
        case '\f':      // ^L - FF - form feed
            line_feed(term);
            break;
        case '\r':      // ^M - CR -  carriage return
            carriage_return(term);
            break;

        case ASCII_CAN: // ^X - CAN - cancel escape sequence
            term->state = S_NORM;
            goto done;
        case '\e':      // ^[ - ESC - start escape sequence
            term->state = S_ESC;
            goto done;

        default:        // everything else
            // ignore unhandled control characters
            if (iscntrl(c)) {
                goto done;
            }

            if (term->line_wrap_pending) {
                carriage_return(term);
                line_feed(term);
            }

            update_char = true;
            update_attr = true;

            // determine character position
            char_pos = xy2pos(term, term->cursor.x, term->cursor.y);

            // advance cursor, and wrap if needed
            term->cursor.x++;
            if (term->cursor.x >= term->cols) {
                term->line_wrap_pending = true;
            }
            break;
    }

write_vga:
    if (update_char) {
        set_fb_char(term, char_pos, c);
    }
    if (update_attr) {
        set_fb_attr(term, char_pos, term->attr);
    }
    if (update_cursor_pos && is_current_terminal(term)) {
        apply_cursor_pos(term);
    }

done:
    clear_bit(&term->printing, 0);
    return;
}

// ----------------------------------------------------------------------------
// private functions

static void esc(struct terminal *term, char c)
{
    //
    // Escape Sequences
    //
    // https://www.man7.org/linux/man-pages/man4/console_codes.4.html
    // https://en.wikipedia.org/wiki/C0_and_C1_control_codes#C1_controls
    //
    switch (c) {
        //
        // C1 sequences
        //
        case 'D':       // ESC D - IND - linefeed (LF)
            line_feed(term);
            break;
        case 'E':       // ESC E - NEL - newline (CRLF)
            carriage_return(term);
            line_feed(term);
            break;
        case 'H':       // ESC H - HTS - set tab stop
            term->tabstops[term->cursor.x] = 1;
            break;
        case 'M':       // ESC M - RI - reverse line feed
            reverse_linefeed(term);
            break;
        case '[':       // ESC [ - CSI - control sequence introducer
            memset(term->csiparam, 0xFF, sizeof(term->csiparam));
            term->paramidx = 0;
            term->state = S_CSI;
            return;

        //
        // "Custom" terminal-related sequences
        //
        case '3':       // ESC 3    disable blink
            term->blink_on = false;
            if (is_current_terminal(term)) {
                apply_blink_state(term);
            }
            break;
        case '4':       // ESC 4    enable blink
            term->blink_on = true;
            if (is_current_terminal(term)) {
                apply_blink_state(term);
            }
            break;
        case '5':       // ESC 5    hide cursor
            term->cursor.hidden = true;
            if (is_current_terminal(term)) {
                apply_cursor_enabled(term);
            }
            break;
        case '6':       // ESC 6    show cursor
            term->cursor.hidden = false;
            if (is_current_terminal(term)) {
                apply_cursor_enabled(term);
            }
            break;
        case '7':       // ESC 7    save terminal
            save_terminal(term);
            break;
        case '8':       // ESC 8    restore terminal
            restore_terminal(term);
            break;
        case 'c':       // ESC c    reset terminal
            reset_terminal(term);
            break;
        case 'h':       // ESC h    clear tab stop
            term->tabstops[term->cursor.x] = 0;     // TODO: replace with ESC [0g (clear current) and ESC [3g (clear all)
            break;
        default:
            break;
    }

    term->line_wrap_pending = false;
    term->state = S_NORM;
}

static void csi(struct terminal *term, char c)
{
    //
    // ANSI Control Sequences
    //
    // https://www.man7.org/linux/man-pages/man3/termios.3.html
    // https://en.wikipedia.org/wiki/ANSI_escape_code
    //

    #define csiparam_clampmin(index,value)      \
    do {                                        \
        if (term->csiparam[index] < (value)) {  \
            term->csiparam[index] = (value);    \
        }                                       \
    } while (0)

    #define csiparam_clampmax(index,value)      \
    do {                                        \
        if (term->csiparam[index] > (value)) {  \
            term->csiparam[index] = (value);    \
        }                                       \
    } while (0)

    switch (c)
    {
        //
        // "Standard" sequences
        //
        case 'A':       // CSI n A  - CUU - move cursor up n rows
            csiparam_clampmin(0, 1);
            cursor_up(term, term->csiparam[0]);
            goto csi_done;
        case 'B':       // CSI n B  - CUD - move cursor down n rows
            csiparam_clampmin(0, 1);
            cursor_down(term, term->csiparam[0]);
            goto csi_done;
        case 'C':       // CSI n C  - CUF - move cursor right (forward) n columns
            csiparam_clampmin(0, 1);
            cursor_right(term, term->csiparam[0]);
            goto csi_done;
        case 'D':       // CSI n D  - CUB - move cursor left (back) n columns
            csiparam_clampmin(0, 1);
            cursor_left(term, term->csiparam[0]);
            goto csi_done;
        case 'E':       // CSI n E  - CNL - move cursor to beginning of line, n rows down
            csiparam_clampmin(0, 1);
            term->cursor.x = 0;
            cursor_down(term, term->csiparam[0]);
            goto csi_done;
        case 'F':       // CSI n F  - CPL - move cursor to beginning of line, n rows up
            csiparam_clampmin(0, 1);
            term->cursor.x = 0;
            cursor_up(term, term->csiparam[0]);
            goto csi_done;
        case 'G':       // CSI n G  - CHA - move cursor to column n
            csiparam_clampmin(0, 1);
            csiparam_clampmax(0, term->cols);
            term->cursor.x = term->csiparam[0] - 1;
            goto csi_done;
        case 'H':       // CSI n ; m H - CUP - move cursor to row n, column m
            csiparam_clampmin(0, 1);
            csiparam_clampmin(1, 1);
            csiparam_clampmax(0, term->rows);
            csiparam_clampmax(1, term->cols);
            term->cursor.y = term->csiparam[0] - 1;
            term->cursor.x = term->csiparam[1] - 1;
            goto csi_done;
        case 'J':       // CSI n J  - ED - erase in display (n = mode)
            csiparam_clampmin(0, 0);
            erase(term, term->csiparam[0]);
            goto csi_done;
        case 'K':       // CSI n K  - EL- erase in line (n = mode)
            csiparam_clampmin(0, 0);
            erase_line(term, term->csiparam[0]);
            goto csi_done;
        case 'S':       // CSI n S  - SU - scroll n lines
            csiparam_clampmin(0, 1);
            scroll(term, term->csiparam[0]);
            goto csi_done;
        case 'T':       // CSI n T  - ST - reverse scroll n lines
            csiparam_clampmin(0, 1);
            scroll(term, -term->csiparam[0]);     // note the negative for reverse!
            goto csi_done;
        case 'm':       // CSI n m  - SGR - set graphics attribute
            for (int i = 0; i <= term->paramidx; i++){
                csiparam_clampmin(i, 0);
                csi_m(term, term->csiparam[i]);
            }
            goto csi_done;

        //
        // Custom (or "private") sequences
        //
        case 's':       // CSI s        save cursor position
            save_cursor(term);
            goto csi_done;
        case 'u':       // CSI u        restore cursor position
            restore_cursor(term);
            goto csi_done;

        //
        // CSI params
        //
        case ';':   // parameter separator
            term->paramidx++;
            if (term->paramidx >= MAX_CSIPARAM) {
                goto csi_done;  // too many params! cancel
            }
            goto csi_next;
        default:    // parameter
            if (isdigit(c)) {
                if (term->csiparam[term->paramidx] == -1) {
                    term->csiparam[term->paramidx] = 0;
                }
                term->csiparam[term->paramidx] *= 10;
                term->csiparam[term->paramidx] += (c - '0');
                goto csi_next;
            }
            goto csi_done;  // invalid param char
    }

csi_done:
    term->line_wrap_pending = false;
    term->state = S_NORM;

csi_next:   // we need more CSI characters; do not alter terminal state
    return;

    #undef csiparam_clampmin
    #undef csiparam_clampmax
}

static void csi_m(struct terminal *term, char p)
{
    static const int CSI_VGA_COLOR_MAP[8] =    // TODO: configure via ioctl??
    {
        // maps ANSI CSI<n>m 3-bit colors to VGA 3-bit color.
        VGA_BLACK,
        VGA_RED,
        VGA_GREEN,
        VGA_YELLOW,
        VGA_BLUE,
        VGA_MAGENTA,
        VGA_CYAN,
        VGA_WHITE
    };

    //
    // Character Attributes via Set Graphics Rendition (SGR) control sequence.
    // CSIm
    //
    // https://www.man7.org/linux/man-pages/man4/console_codes.4.html
    // https://en.wikipedia.org/wiki/ANSI_escape_code
    //

    switch (p) {
        case 0:     // reset to defaults
            term->attr = term->csi_defaults.attr;
            break;
        case 1:     // set bright foreground (bold)
            term->attr.bold = true;
            break;
        case 2:     // set faint (simulated with color)
            term->attr.faint = true;
            break;
        case 3:     // set italic (simulated with color)
            term->attr.italic = true;
            break;
        case 4:     // set underline (simulated with color)
        case 21:    // double underline not supported on VGA, alias as normal underline
            term->attr.underline = true;
            break;
        case 5:     // set blink
        case 6:     // rapid blink not supported on VGA; alias as normal blink
            term->attr.blink = true;
            break;
        case 7:     // set fg/bg color inversion
            term->attr.invert = true;
            break;
        case 8:     // hide text
            term->attr.conceal = true;
            break;
        case 9:     // strikethrough (simulated with color)
            term->attr.strike = true;
            break;
        case 22:    // normal intensity (neither bright nor faint)
            term->attr.bold = false;
            term->attr.faint = false;
            break;
        case 23:    // disable italic
            term->attr.italic = false;
            break;
        case 24:    // disable underline
            term->attr.underline = false;
            break;
        case 25:    // disable blink
            term->attr.blink = false;
            break;
        case 27:    // disable fg/bg inversion
            term->attr.invert = false;
            break;
        case 28:    // reveal text
            term->attr.conceal = false;
            break;
        case 29:    // disable strikethrough
            term->attr.strike = false;
            break;
        default:
            // colors
            if (p >= 30 && p <= 37) {
                term->attr.fg = CSI_VGA_COLOR_MAP[p - 30];
            }
            else if (p >= 40 && p <= 47) {
                term->attr.bg = CSI_VGA_COLOR_MAP[p - 40];
            }
            else if (p == 39) {
                term->attr.fg = term->csi_defaults.attr.fg;
            }
            else if (p == 49) {
                term->attr.bg = term->csi_defaults.attr.bg;
            }
            else if (p >= 90 && p <= 97) {
                term->attr.fg = VGA_BRIGHT | CSI_VGA_COLOR_MAP[p - 90];
            }
            else if (p >= 100 && p <= 107) {
                term->attr.bg = VGA_BRIGHT | CSI_VGA_COLOR_MAP[p - 100];
            }
            break;
    }
}

static void reset_terminal(struct terminal *term)
{
    terminal_defaults(term);
    erase(term, ERASE_ALL);
    if (is_current_terminal(term)) {
        apply_vga_state(term);
    }
}

static void save_terminal(struct terminal *term)
{
    struct terminal_save_state *save = &term->saved_state;

    memcpy(save->tabstops, term->tabstops, MAX_TABSTOP);
    save->blink_on = term->blink_on;
    save->attr = term->attr._value;
    save->cursor = term->cursor._value;
}

static void restore_terminal(struct terminal *term)
{
    struct terminal_save_state *save = &term->saved_state;

    memcpy(term->tabstops, save->tabstops, MAX_TABSTOP);
    term->blink_on = save->blink_on;
    term->attr._value = save->attr;
    term->cursor._value = save->cursor;

    if (is_current_terminal(term)) {
        apply_vga_state(term);
    }
}

static void save_cursor(struct terminal *term)
{
    term->saved_state.cursor = term->cursor._value;
}

static void restore_cursor(struct terminal *term)
{
    term->cursor._value = term->saved_state.cursor;
    if (is_current_terminal(term)) {
        apply_cursor_state(term);
    }
}

static void backspace(struct terminal *term)
{
    cursor_left(term, 1);
    term->line_wrap_pending = false;
}

static void carriage_return(struct terminal *term)
{
    term->cursor.x = 0;
    term->line_wrap_pending = false;
}

static void line_feed(struct terminal *term)
{
    if (++term->cursor.y >= term->rows) {
        scroll(term, 1);
        term->cursor.y--;
    }
    term->line_wrap_pending = false;
}

static void reverse_linefeed(struct terminal *term)
{
    if (term->cursor.y-- <= 0) {
        scroll(term, -1);
        term->cursor.y = 0;
    }
}

static void tab(struct terminal *term)
{
    assert(MAX_TABSTOP == term->cols);

    while (term->cursor.x < MAX_TABSTOP - 1) {
        if (term->tabstops[++term->cursor.x]) {
            break;
        }
    }

    if (term->cursor.x >= MAX_TABSTOP - 1) {
        term->cursor.x = MAX_TABSTOP - 1;
    }
}

static void scroll(struct terminal *term, int n)   // n < 0 is reverse scroll
{
    if (n == 0) return;

    bool reverse = (n < 0);
    if (reverse) n = -n;

    if (n > term->rows) n = term->rows;

#if FAST_SCROLL
    int n_blank = n * term->cols;
    int n_visible = term->cols * term->rows;
    int n_kept = n_visible - n_blank;
    int last_row_start = VGA_FB_WORDS - (VGA_FB_WORDS % term->cols);

    term->origin += (reverse) ? -n_blank : n_blank;
    if (term->origin + n_visible >= last_row_start) {   // will wrap if negative
        if (reverse) {
            memmove(term->framebuf + ((last_row_start - n_visible) << 1), term->framebuf, n_kept << 1);
            term->origin = last_row_start - n_visible - n_blank;
        }
        else {
            memmove(term->framebuf, term->framebuf + ((last_row_start - n_visible) << 1), n_kept << 1);
            term->origin = 0;
        }
    }

    for (int i = 0; i < n_blank; i++) {
        int pos = (reverse) ? i : n_visible - n_blank + i;
        set_fb_char(term, pos, ' ');
        set_fb_attr(term, pos, term->attr);
    }

    if (is_current_terminal(term)) {
#if VSYNC_SCROLL
    vga_wait_for_vsync();   // makes scrolling sllooowww...
#endif
        vga_set_scan_start(term->origin);
    }

#else

    int n_blank = n * term->cols;
    int n_cells = (term->rows * term->cols) - n_blank;
    int n_bytes = n_cells * sizeof(struct vga_cell);

    void *src_end = &((struct vga_cell *) term->framebuf)[n_blank];
    void *src = (reverse) ? term->framebuf : src_end;
    void *dst = (reverse) ? src_end : term->framebuf;
    memmove(dst, src, n_bytes);

    for (int i = 0; i < n_blank; i++) {
        int pos = (reverse) ? i : n_cells + i;
        set_fb_char(term, pos, ' ');
        set_fb_attr(term, pos, term->attr);
    }
#endif
}

static void erase(struct terminal *term, int mode)
{
    int start;
    int count;
    int pos = xy2pos(term, term->cursor.x, term->cursor.y);
    int area = term->rows * term->cols;

    switch (mode) {
        case ERASE_DOWN:    // erase screen from cursor down
            start = pos;
            count = area - pos;
            break;
        case ERASE_UP:      // erase screen from cursor up
            start = 0;
            count = pos + 1;
            break;
        case ERASE_ALL:     // erase entire screen
        default:
            start = 0;
            count = area;
            break;
    }

    for (int i = 0; i < count; i++) {
        set_fb_char(term, start + i, ' ');
        set_fb_attr(term, start + i, term->attr);
    }
}

static void erase_line(struct terminal *term, int mode)
{
    int start;
    int count;
    int pos = xy2pos(term, term->cursor.x, term->cursor.y);

    switch (mode) {
        case ERASE_DOWN:    // erase line from cursor forward
            start = pos;
            count = term->cols - (pos % term->cols);
            break;
        case ERASE_UP:      // erase line from cursor back
            start = xy2pos(term, 0, term->cursor.y);
            count = (pos % term->cols) + 1;
            break;
        case ERASE_ALL:     // erase entire line
        default:
            start = xy2pos(term, 0, term->cursor.y);
            count = term->cols;
    }

    for (int i = 0; i < count; i++) {
        set_fb_char(term, start + i, ' ');
        set_fb_attr(term, start + i, term->attr);
    }
}

static void cursor_up(struct terminal *term, int n)
{
    if (term->cursor.y - n > 0) {
        term->cursor.y -= n;
    }
    else {
        term->cursor.y = 0;
    }
}

static void cursor_down(struct terminal *term, int n)
{
    if (term->cursor.y + n < term->rows - 1) {
        term->cursor.y += n;
    }
    else {
        term->cursor.y = term->rows - 1;
    }
}

static void cursor_left(struct terminal *term, int n)
{
    if (term->cursor.x - n > 0) {
        term->cursor.x -= n;
    }
    else {
        term->cursor.x = 0;
    }
}

static void cursor_right(struct terminal *term, int n)
{
    if (term->cursor.x + n < term->cols - 1) {
        term->cursor.x += n;
    }
    else {
        term->cursor.x = term->cols - 1;
    }
}

static uint16_t xy2pos(const struct terminal *term, uint16_t x, uint16_t y)
{
    if (x > term->cols - 1) {
        x = term->cols - 1;
    }
    if (y > term->rows - 1) {
        y = term->rows - 1;
    }

    return y * term->cols + x;
}

static void pos2xy(struct terminal *term, uint16_t pos)
{
    pos = (pos - term->origin + VGA_FB_WORDS) % VGA_FB_WORDS;
    term->cursor.x = pos % term->cols;
    term->cursor.y = pos / term->cols;
}

static void set_fb_char(struct terminal *term, uint16_t pos, char c)
{
    pos = (term->origin + pos) % VGA_FB_WORDS;
    ((struct vga_cell *) term->framebuf)[pos].ch = c;
}

static void set_fb_attr(struct terminal *term, uint16_t pos, struct _char_attr attr)
{
    struct vga_attr *vga_attr;

    pos = (term->origin + pos) % VGA_FB_WORDS;
    vga_attr = &((struct vga_cell *) term->framebuf)[pos].attr;

    vga_attr->bg = attr.bg & 0xF;
    vga_attr->fg = attr.fg & 0xF;

    if (attr.bold) {
        vga_attr->fg |= VGA_BRIGHT;
    }
    if (attr.faint) {   // TODO: ioctl configure color
        vga_attr->fg = VGA_BRIGHT | VGA_BLACK;
    }
    if (attr.underline) {
        vga_attr->fg = VGA_CYAN;
    }
    if (attr.italic) {
        vga_attr->fg = VGA_GREEN;
    }
    if (attr.strike) {
        vga_attr->fg = VGA_MAGENTA;
    }
    if (attr.invert) {
        swap(vga_attr->bg, vga_attr->fg);
    }
    if (attr.conceal) {
        vga_attr->fg = vga_attr->bg;
    }
    if (attr.blink) {
        vga_attr->bg |= VGA_BRIGHT; // bright becomes blink when enabled
    }
}

static void map_terminal_fb(struct terminal *term, uintptr_t phys)
{
    update_page_mappings((uintptr_t) term->framebuf, phys,
        VGA_FB_SIZE_PAGES, _PAGE_WRITABLE | _PAGE_PRESENT);
}

static void apply_blink_state(const struct terminal *term)
{
    vga_enable_blink(term->blink_on);
}

static void apply_cursor_enabled(const struct terminal *term)
{
    vga_enable_cursor(!term->cursor.hidden);
}

static void apply_cursor_pos(const struct terminal *term)
{
    uint16_t pos;

    pos = term->origin + xy2pos(term, term->cursor.x, term->cursor.y);
    vga_set_cursor_pos(pos);
}

static void apply_cursor_shape(const struct terminal *term)
{
    vga_set_cursor_shape(term->cursor.shape);
}

static void apply_cursor_state(const struct terminal *term)
{
    apply_cursor_enabled(term);
    apply_cursor_shape(term);
    apply_cursor_pos(term);
}

static void apply_vga_state(const struct terminal *term)
{
    apply_blink_state(term);
    apply_cursor_state(term);

#if VSYNC
    vga_wait_for_vsync();
#endif
    vga_set_scan_start(term->origin);
}
