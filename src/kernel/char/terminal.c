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
 *      Created: March 26, 2023, big overhaul in Fall 2026
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
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

#define VGA_FB_SELECT           VGA_MEMORY_32K_HI               // B8000-BFFFF
#define VGA_FB_SIZE             (32 << KB_SHIFT)                // total VGA memory in bytes
#define VGA_FB_PAGES            2                               // num frame buffer pages (not MMU pages)
#define VT_FB_SIZE              (VGA_FB_SIZE / VGA_FB_PAGES)    // visible frame buffer size
#define VT_FB_WORDS             (VT_FB_SIZE / sizeof(struct vga_cell)) // chars in frame buffer

static_assert(VGA_FB_SELECT == VGA_MEMORY_32K_HI);  // only supported region for now
static_assert(VGA_FB_PAGES == 2);                   // for double buffering

// TODO: rename to 'vgaterm.c'
// TODO: blank screen after some set amount of time
// TODO: smooth scroll? :D
// TODO: Shift+PgUp / Shift+PgDn for terminal scrollback

static int s_curr_page = 0;
static int s_curr_term = CONFIG_INITIAL_VT;
static int s_switching = 0;
static struct terminal s_terms[NR_TERMINAL];
static struct vga_fb_info s_vga_fb;

#define get_curr_hwfb_page() (s_vga_fb.ptr + (s_curr_page * VT_FB_WORDS))

static void initialize_terminal(int num, struct terminal *term);
static void terminal_defaults(struct terminal *term);
static bool is_current_terminal(struct terminal *term);
static int write_to_terminal(struct terminal *term, const char *buf, size_t count);
static uint16_t xy2pos(const struct terminal *term, uint16_t x, uint16_t y);
static void pos2xy(struct terminal *term, uint16_t pos);

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

    int minor = _DEV_MIN(tty->device);
    if (minor < TTY_MIN || minor > TTY_MAX) {
        return -ENXIO;  // TTY device is not a virtual terminal
    }

    *term = get_terminal(1 + __tty_driver_index(tty));
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

static size_t terminal_tty_write_room(struct tty *tty)
{
    // we can write the frame buffer forever...
    // return something sufficiently large to satisfy ldisc logic
    return (tty->stopped) ? 0 : 4096;
}

static int terminal_tty_ioctl(struct tty *tty, int op, void *arg)
{
    return -ENOTTY;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// console implementation

#if CONFIG_VT_CONSOLE

static dev_t vt_console_device(struct console *cons)
{
    return __mkttydev((cons->number) ? cons->number : s_curr_term);
}

__init void init_early_terminal(int num, struct terminal *term)
{
    // inherit virtual terminal properties from VGA, grab the actual frame
    // buffer address currently in use; early terminal writes VGA directly
    vga_get_fb_info(&s_vga_fb);
    initialize_terminal(num, term);
    pos2xy(term, vga_get_cursor_pos());
    term->cursor.shape = vga_get_cursor_shape();
    term->backbuf = NULL;   // allocated later by init_terminal_driver()
    s_curr_term = num;

    // give ourselves a fresh line
    __terminal_putc(term, '\r');
    __terminal_putc(term, '\n');
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
    int c;
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
    .number = CONFIG_VT_CONSOLE_NUM,
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

enum seq_state {
    S_NORM,
    S_ESC,
    S_CSI,
    S_DEC,
    S_PVT,
};

enum erase_mode {
    ERASE_DOWN,
    ERASE_UP,
    ERASE_ALL
};

// terminal state
static void reset_terminal(struct terminal *term);                  // ESC c
static void save_terminal(struct terminal *term);                   // ESC 7
static void restore_terminal(struct terminal *term);                // ESC 8
static void save_cursor_pos(struct terminal *term);                 // ESC [s
static void restore_cursor_pos(struct terminal *term);              // ESC [u
static void reverse_video(struct terminal *term);                   // ESC [?5h/l
static void enable_blink(struct terminal *term);                    // ESC [=5~
static void disable_blink(struct terminal *term);                   // ESC [=6~
static void blank_screen(struct terminal *term);                    // ESC [=2~
static void unblank_screen(struct terminal *term);                  // ESC [=3~
static void set_autoblank_timeout(struct terminal *term, int min);  // ESC [=4;<n>~
static void set_autoblank(struct terminal *term);
static void clear_autoblank(struct terminal *term);
static void rearm_autoblank(struct terminal *term);
static void show_cursor(struct terminal *term);                     // ESC [?25h
static void hide_cursor(struct terminal *term);                     // ESC [?25l

// character handling
static void esc_sequence(struct terminal *term, unsigned char c);   // ^[ (ESC)
static void csi_sequence(struct terminal *term, unsigned char c);   // ESC [
static void sgr_sequence(struct terminal *term, unsigned char c);   // ESC [m
static void dec_sequence(struct terminal *term, unsigned char c);   // ESC [?
static void pvt_sequence(struct terminal *term, unsigned char c);   // ESC [=
static void backspace(struct terminal *term);                       // ^H
static void tab(struct terminal *term);                             // ^I
static void line_feed(struct terminal *term);                       // ^J
static void reverse_linefeed(struct terminal *term);                // ESC M
static void carriage_return(struct terminal *term);                 // ^M
static void scroll(struct terminal *term, int n);                   // ESC [<n>S/T
static void erase(struct terminal *term, int mode);                 // ESC [<n>J
static void erase_line(struct terminal *term, int mode);            // ESC [<n>K
static void insert_tabstop(struct terminal *term);                  // ESC H
static void clear_tabstop(struct terminal *term, int mode);         // ESC [<n>g
static void default_tabstops(struct terminal *term);                // ESC [2g
static void cursor_up(struct terminal *term, int n);                // ESC [<n>A
static void cursor_down(struct terminal *term, int n);              // ESC [<n>B
static void cursor_right(struct terminal *term, int n);             // ESC [<n>C
static void cursor_left(struct terminal *term, int n);              // ESC [<n>D
static void cursor_style(struct terminal *term, int cursor);        // ESC [<n>q

// physical VGA state
static void apply_display_blink(const struct terminal *term);
static void apply_display_blank(const struct terminal *term);
static void apply_display_state(const struct terminal *term);
static void apply_cursor_hidden(const struct terminal *term);
static void apply_cursor_pos(const struct terminal *term);
static void apply_cursor_shape(const struct terminal *term);
static void apply_cursor_state(const struct terminal *term);
static void apply_vga_state(const struct terminal *term);

static int sgr_to_vga_color(int sgr_color);

// frame buffer
static struct vga_cell * get_terminal_fb(struct terminal *term);
static void set_fb_cell(struct terminal *term, uint16_t pos, char c);
static void set_sgr(const struct terminal *term, struct vga_cell *cell, struct _sgr_attr *sgr);

static void /*__isr*/ autoblank_timer(int irq, struct iregs *regs);

// ----------------------------------------------------------------------------
// initialization

__init void init_terminal_driver(void)
{
    // set frame buffer address
    vga_set_fb_base(VGA_FB_SELECT);
    vga_get_fb_info(&s_vga_fb);

    // ensure VGA aperture mapped uncached  (TODO: improve this interface)
    update_page_mappings(s_vga_fb.ptr, s_vga_fb.phys_ptr, (VT_FB_SIZE >> PAGE_SHIFT),
        _PAGE_PRESENT | _PAGE_WRITABLE | _PAGE_PCD | _PAGE_PWT);

    pr_debug("vga: frame buffer %u pages %p-%p\n",
        s_vga_fb.size_pages, KERNEL_ADDR(s_vga_fb.phys_ptr),
        KERNEL_ADDR(s_vga_fb.phys_ptr + (s_vga_fb.size_pages << PAGE_SHIFT)-1));

    uint16_t rows = vga_get_rows();
    uint16_t cols = vga_get_cols();

    pr_debug("vga: window size %dx%x\n", cols, rows);
    assert(cols <= MAX_TABSTOP); // shouldn't be an issue for a while...

#if EIGHT_DOT_MODE
    vga_seqr_write(VGA_SEQR_REG_CLOCKING, vga_seqr_read(VGA_SEQR_REG_CLOCKING) | 0x1);  // 9/8 Dot Mode
#endif

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
        term->backbuf = alloc_pages(MEM_ZERO, get_order(VT_FB_SIZE));
        if (!term->backbuf) {
            panic("unable to allocate frame buffer for terminal %d!", i);
        }

        if (term->number == s_curr_term) {
            // preserve boot output by copying VGA into back buffer,
            // then map the default terminal's frame buffer into VGA memory
            memcpy(term->backbuf, get_curr_hwfb_page(), VT_FB_SIZE);
            pos2xy(term, vga_get_cursor_pos());
            term->cursor.shape = vga_get_cursor_shape();
        }
        else {
            char buf[32];   // TODO: probably have shell/login program print this
            write_to_terminal(term, buf,
                snprintf(buf, sizeof(buf), "\e[2J\e[Htty%d\r\n", i));
        }
    }

    // register the terminal TTY driver, needed for terminal switch
    if (tty_register_driver(&terminal_driver)) {
        panic("unable to register terminal driver!");
    }

#if CONFIG_VT_CONSOLE
    // register the virtual terminal console
    register_console(&vt_console);
#endif

    irq_register(IRQ_TIMER, autoblank_timer);

    // switch to configured default terminal
    if (s_curr_term != CONFIG_INITIAL_VT) {
        if (switch_terminal(CONFIG_INITIAL_VT)) {
            panic("unable to switch terminal from %d to %d", s_curr_term, CONFIG_INITIAL_VT);
        }
    }
}

static void initialize_terminal(int num, struct terminal *term)
{
    if (term->initialized) {
        return;
    }

    if (num == 0) {
        num = s_curr_term;
    }

    term->number = num;
    term->cols = min(vga_get_cols(), MAX_TABSTOP);
    term->rows = vga_get_rows();
    term->defaults.bell.hz = DEFAULT_BELL_HZ;
    term->defaults.bell.ms = DEFAULT_BELL_MS;
    term->defaults.cursor_style = DEFAULT_CURSOR_STYLE;
    term->defaults.sgr.bg = DEFAULT_BG_COLOR;
    term->defaults.sgr.fg = DEFAULT_FG_COLOR;
    term->defaults.sgr.faint = DEFAULT_FAINT_COLOR;
    term->defaults.sgr.italic = DEFAULT_ITALIC_COLOR;
    term->defaults.sgr.undline = DEFAULT_UNDERLINE_COLOR;
    term->defaults.sgr.strike = DEFAULT_STRIKE_COLOR;
    term->defaults.autoblank_interval = DEFAULT_BLANK_TIME;
    terminal_defaults(term);
    term->initialized = true;
}

static void terminal_defaults(struct terminal *term)
{
    // NOTE: do not memset the whole struct!!
    // TODO: might be good to factor all the mutable stuff into a vt_state
    //       struct so we can just zero that only instead of having to itemize
    //       (and potentially forget) mutable fields

    default_tabstops(term);
    memset(term->csiparam, -1, sizeof(term->csiparam));
    term->paramidx = 0;
    term->seq_state = S_NORM;
    memset(&term->disp, 0, sizeof(struct _display_state));
    term->disp.autowrap = true;
    set_autoblank_timeout(term, term->defaults.autoblank_interval);
    term->sgr._value = 0;
    term->sgr.bg = term->defaults.sgr.bg;
    term->sgr.fg = term->defaults.sgr.fg;
    memset(&term->cursor, 0, sizeof(struct _cursor));
    cursor_style(term, term->defaults.cursor_style);
    save_terminal(term);
}

// ----------------------------------------------------------------------------
// public functions

int switch_terminal(int num)
{
    if (num <= 0 || num > NR_TERMINAL) {
        return -EINVAL;
    }
    if (test_and_set_bit(&s_switching, 0)) {
        return -EBUSY;
    }

    struct terminal *curr = get_terminal(0);
    struct terminal *next = get_terminal(num);
    int ret = 0;

    if (curr == next) {
        goto switch_unlock;
    }
    if (!next->initialized || !curr->backbuf || !next->backbuf) {
        ret = -EIO;
        goto switch_unlock;
    }

    irq_mask(IRQ_KEYBOARD);
    klog_defer();

    // swap frame buffers
    memcpy(curr->backbuf, get_curr_hwfb_page(), VT_FB_SIZE);
    s_curr_term = next->number;
    s_curr_page = (s_curr_page & 1) ^ 1;
    memcpy(get_curr_hwfb_page(), next->backbuf, VT_FB_SIZE);

    // apply display state
    rearm_autoblank(next);
    apply_vga_state(next);

    // preserve current held-down key state, apply other terminal's keyboard state
    next->kb_state._modkeys = curr->kb_state._modkeys;
    ps2kb_apply_state(&next->kb_state.hw_state);

    klog_resume();
    irq_unmask(IRQ_KEYBOARD);

switch_unlock:
    clear_bit(&s_switching, 0);
    return ret;
}

struct terminal * get_terminal(int num)
{
    if (num < 0 || num > NR_TERMINAL) {
        panic("attempt to get nonexistant terminal %d!", num);
    }
    if (num == 0) {
        num = s_curr_term;
    }
    assert(num > 0);

    struct terminal *term = &s_terms[num - 1];
    if (term->initialized) {
        assert(term->number == num);
    }

    return term;
}

void poke_blanked_terminal(struct terminal *term)
{
    rearm_autoblank(term);
}

// ----------------------------------------------------------------------------
// private functions

static bool is_current_terminal(struct terminal *term)
{
    return term->number == s_curr_term;
}

static int write_to_terminal(struct terminal *term, const char *buf, size_t count)
{
    const char *p;

    if (!term || !buf) {
        return -EINVAL;
    }

    p = buf;
    while (p < buf + count) {
#if CONFIG_E9_HACK && CONFIG_E9_HACK_VT
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
        // TODO: preserve the char and defer print
        return;
    }

    // TODO: ioctl to prevent processing of control characters
    // so we can write to the VGA directly

    // handle escape sequences if not a control character
    if (!iscntrl(c)) {
        switch (term->seq_state) {
            case S_ESC:
                esc_sequence(term, c);
                goto write_vga;
            case S_CSI:
                csi_sequence(term, c);
                goto write_vga;
            case S_DEC:
                dec_sequence(term, c);
                goto write_vga;
            case S_PVT:
                pvt_sequence(term, c);
                goto write_vga;
            case S_NORM:
                break;
            default:
                term->seq_state = S_NORM;
                break;
        }
    }

    // control chars received while an escape sequence is in progress must not
    // be executed; only ESC (restart) and CAN (cancel) have meaning here, and
    // both are handled in the switch below
    if (iscntrl(c) && term->seq_state != S_NORM && c != '\e' && c != ASCII_CAN) {
        goto done;
    }

    // control characters
    switch (c) {
        case '\a':      // ^G - BEL - beep!
            beep(term->defaults.bell.hz, term->defaults.bell.ms, false);
            break;
        case '\b':      // ^H - BS  - backspace
            backspace(term);
            break;
        case '\t':      // ^I - HT  - horizontal tab
            tab(term);
            break;
        case '\n':      // ^J - LF  - line feed
            __fallthrough;
        case '\v':      // ^K - VT  - vertical tab
        case '\f':      // ^L - FF  - form feed
            line_feed(term);
            break;
        case '\r':      // ^M - CR  - carriage return
            carriage_return(term);
            break;

        case ASCII_CAN: // ^X - CAN - cancel escape sequence
            term->seq_state = S_NORM;
            goto done;
        case '\e':      // ^[ - ESC - start escape sequence
            term->seq_state = S_ESC;
            goto done;

        default:        // everything else
            // ignore unhandled control characters
            if (iscntrl(c)) {
                goto done;
            }

            // handle autowrap behavior
            if (term->disp.pending_wrap && term->disp.autowrap) {
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
                term->disp.pending_wrap = true;
            }
            break;
    }

write_vga:
    if (update_attr || update_char) {
        set_fb_cell(term, char_pos, (update_char) ? c : '\0');
    }
    if (is_current_terminal(term) && update_cursor_pos) {
        apply_cursor_pos(term);
    }
    poke_blanked_terminal(term);

done:
    clear_bit(&term->printing, 0);

    // handle pending terminal switch
    if (term->disp.switch_to > 0) {
        int nr = term->disp.switch_to;
        term->disp.switch_to = 0;
        (void) switch_terminal(nr);
    }
    return;
}

#define param_clamp_min(index,value)        \
do {                                        \
    if (term->csiparam[index] < (value)) {  \
        term->csiparam[index] = (value);    \
    }                                       \
} while (0)

#define param_clamp_max(index,value)        \
do {                                        \
    if (term->csiparam[index] > (value)) {  \
        term->csiparam[index] = (value);    \
    }                                       \
} while (0)

#define param_set_if_lt(index,value, lhs)   \
do {                                        \
    if (term->csiparam[index] < (value)) {  \
        (lhs) = term->csiparam[index];      \
    }                                       \
} while (0)


static void esc_sequence(struct terminal *term, unsigned char c)
{
    //
    // Non-CSI Escape Sequences
    //
    // https://www.man7.org/linux/man-pages/man4/console_codes.4.html
    // https://en.wikipedia.org/wiki/C0_and_C1_control_codes#C1_controls
    //
    switch (c) {
        //
        // C1 Sequences
        //
        case 'D':       // ESC D    - IND   - linefeed (LF)
            line_feed(term);
            break;
        case 'E':       // ESC E    - NEL   - newline (CRLF)
            carriage_return(term);
            line_feed(term);
            break;
        case 'H':       // ESC H    - HTS   - set tab stop
            insert_tabstop(term);
            break;
        case 'M':       // ESC M    - RI    - reverse line feed
            reverse_linefeed(term);
            break;
        case '[':       // ESC [    - CSI   - control sequence introducer
            memset(term->csiparam, -1, sizeof(term->csiparam));
            term->paramidx = 0;
            term->seq_state = S_CSI;
            return;

        //
        // VT-100 Sequences
        //
        case '7':       // ESC 7    - DECSC - save terminal state (cursor/attr/tabs/blink)
            save_terminal(term);
            break;
        case '8':       // ESC 8    - DECRC - restore terminal state
            restore_terminal(term);
            break;
        case 'c':       // ESC c    - RIS   - reset terminal (defaults and erase)
            reset_terminal(term);
            break;

        default:
            break;
    }

    term->disp.pending_wrap = false;
    term->seq_state = S_NORM;
}

static void csi_sequence(struct terminal *term, unsigned char c)
{
    //
    // CSI ("ANSI") Escape Sequences
    //
    // https://www.man7.org/linux/man-pages/man3/termios.3.html
    // https://en.wikipedia.org/wiki/ANSI_escape_code
    //

    switch (c)
    {
        //
        // Standard VT-100 / ANSI Sequences
        //
        case 'A':       // ESC [<n>A    - CUU   - move cursor up n rows
            param_clamp_min(0, 1);
            cursor_up(term, term->csiparam[0]);
            goto csi_done;
        case 'B':       // ESC [<n>B    - CUD   - move cursor down n rows
            param_clamp_min(0, 1);
            cursor_down(term, term->csiparam[0]);
            goto csi_done;
        case 'C':       // ESC [<n>C    - CUF   - move cursor right (forward) n columns
            param_clamp_min(0, 1);
            cursor_right(term, term->csiparam[0]);
            goto csi_done;
        case 'D':       // ESC [<n>D    - CUB   - move cursor left (back) n columns
            param_clamp_min(0, 1);
            cursor_left(term, term->csiparam[0]);
            goto csi_done;
        case 'E':       // ESC [<n>E    - CNL   - move cursor to beginning of line, n rows down
            param_clamp_min(0, 1);
            term->cursor.x = 0;
            cursor_down(term, term->csiparam[0]);
            goto csi_done;
        case 'F':       // ESC [<n>F    - CPL   - move cursor to beginning of line, n rows up
            param_clamp_min(0, 1);
            term->cursor.x = 0;
            cursor_up(term, term->csiparam[0]);
            goto csi_done;
        case 'G':       // ESC [<n>G    - CHA   - move cursor to column n
            param_clamp_min(0, 1);
            param_clamp_max(0, term->cols);
            term->cursor.x = term->csiparam[0] - 1;
            goto csi_done;
        case 'H':       // ESC [<n>;<m>H - CUP  - move cursor to row n, column m
            param_clamp_min(0, 1);
            param_clamp_max(0, term->rows);
            param_clamp_min(1, 1);
            param_clamp_max(1, term->cols);
            term->cursor.y = term->csiparam[0] - 1;
            term->cursor.x = term->csiparam[1] - 1;
            goto csi_done;
        case 'J':       // ESC [<n>J    - ED    - erase in display (n: 0=down, 1=up, 2=all)
            param_clamp_min(0, 0);
            erase(term, term->csiparam[0]);
            goto csi_done;
        case 'K':       // ESC [<n>K    - EL    - erase in line (n: 0=forward, 1=back, 2=all)
            param_clamp_min(0, 0);
            erase_line(term, term->csiparam[0]);
            goto csi_done;
        case 'g':       // ESC [<n>g    - TBC   - clear tab stop (n: 0=here, 3=all)
            param_clamp_min(0, 0);
            clear_tabstop(term, term->csiparam[0]);
            goto csi_done;
        case 'm':       // ESC [<...>m  - SGR   - set graphics attribute
            for (int i = 0; i <= term->paramidx; i++){
                param_clamp_min(i, 0);
                sgr_sequence(term, term->csiparam[i]);
            }
            goto csi_done;

        //
        // Common Private Sequences
        //
        case 'S':       // ESC [<n>S    - SU    - scroll n lines
            param_clamp_min(0, 1);
            scroll(term, term->csiparam[0]);
            goto csi_done;
        case 'T':       // ESC [<n>T    - ST    - reverse scroll n lines
            param_clamp_min(0, 1);
            scroll(term, -1 * term->csiparam[0]);   // note: negative for reverse!
            goto csi_done;
        case 'q':       // ESC [<n>q    - SCUSR - set cursor style
            param_clamp_min(0, 1);
            cursor_style(term, term->csiparam[0]);
            break;
        case 's':       // ESC [s       - SCOSC - save cursor position
            save_cursor_pos(term);
            goto csi_done;
        case 'u':       // ESC [u       - SCORC - restore cursor position
            restore_cursor_pos(term);
            goto csi_done;

        //
        // Private VT-100 / VT-220 Sequences
        //
        case '?':       // ESC [?       - DEC private "set/reset mode" sequence
            memset(term->csiparam, -1, sizeof(term->csiparam));
            term->paramidx = 0;
            term->seq_state = S_DEC;
            return;

        //
        // Private OH-WES Sequences
        //
        case '=':       // ESC [=       - OH-WES private sequence
            memset(term->csiparam, -1, sizeof(term->csiparam));
            term->paramidx = 0;
            term->seq_state = S_PVT;
            return;

        //
        // CSI parameter handling
        //
        case ';':   // parameter separator
            term->paramidx++;
            if (term->paramidx >= MAX_CSIPARAM) {
                goto csi_done;  // too many params! cancel
            }
            goto csi_next;
        default:    // parameter
            if (isdigit(c)) {
                int *param = &term->csiparam[term->paramidx];
                if (*param == -1) {
                    *param = 0;
                }
                // prevent overflow
                if (*param <= (INT_MAX - (c - '0')) / 10) {
                    *param *= 10;
                    *param += (c - '0');
                }
                goto csi_next;
            }
            goto csi_done;  // invalid param char
    }

csi_done:
    term->disp.pending_wrap = false;
    term->seq_state = S_NORM;

csi_next:   // we need more CSI characters; do not alter terminal state
    return;
}

static void dec_sequence(struct terminal *term, unsigned char c)
{
    bool set_mode;
    int *param = &term->csiparam[term->paramidx];

    if (isdigit(c)) {
        if (*param == -1) {
            *param = 0;
        }
        // prevent overflow
        if (*param <= (INT_MAX - (c - '0')) / 10) {
            *param *= 10;
            *param += (c - '0');
        }
        goto dec_next;
    }

    switch (c) {
        case ';':   // parameter separator
            term->paramidx++;
            if (term->paramidx >= MAX_CSIPARAM) {
                goto dec_done;  // too many params! cancel
            }
            goto dec_next;
        case 'h':   // ESC [?<n>h       - DECSET    - set mode
            set_mode = true;
            break;
        case 'l':   // ESC [?<n>l       - DECRST    - reset mode
            set_mode = false;
            break;
        default:
            goto dec_done;  // invalid DEC private sequence terminator
    }

    for (int i = 0; i <= term->paramidx; i++) {
        switch (term->csiparam[i]) {
            case 5:     // ESC [?5h/l   - DECSCNM   - reverse/normalize all colors on screen
                if ((set_mode && !term->disp.reverse_video) ||
                   (!set_mode && term->disp.reverse_video)) {
                    reverse_video(term);
                }
                break;
            case 7:     // ESC [?7h/l   - DECAWM    - enable/disable autowrap
                term->disp.autowrap = set_mode;
                break;
            case 25:    // ESC [?25h/l  - DECTCEM   - show/hide VGA text mode cursor
                if (set_mode) {
                    show_cursor(term);
                }
                else {
                    hide_cursor(term);
                }
                break;
            default:
                break;
        }
    }

dec_done:
    term->disp.pending_wrap = false;
    term->seq_state = S_NORM;

dec_next:   // we need more escape sequence chars; do not alter terminal state
    return;
}

static void pvt_sequence(struct terminal *term, unsigned char c)
{
    int *param = &term->csiparam[term->paramidx];

    if (isdigit(c)) {
        if (*param == -1) {
            *param = 0;
        }
        // prevent overflow
        if (*param <= (INT_MAX - (c - '0')) / 10) {
            *param *= 10;
            *param += (c - '0');
        }
        goto pvt_next;
    }

    switch (c) {
        case ';':   // parameter separator
            term->paramidx++;
            if (term->paramidx >= MAX_CSIPARAM) {
                goto pvt_done;  // too many params! cancel
            }
            goto pvt_next;
        case '~':   // private sequence terminator
            break;
        default:
            goto pvt_done;  // invalid OH-WES private sequence terminator
    }

    int fn = term->csiparam[0];
    switch (fn) {
        case 0:     // ESC [=0;<m>;<n>~     - set default color rendition for attribute m to n
            switch (term->csiparam[1]) {
                case 0: param_set_if_lt(2, 16, term->defaults.sgr.fg);      break;
                case 1: param_set_if_lt(2, 16, term->defaults.sgr.bg);      break;
                case 2: param_set_if_lt(2, 16, term->defaults.sgr.faint);   break;
                case 3: param_set_if_lt(2, 16, term->defaults.sgr.italic);  break;
                case 4: param_set_if_lt(2, 16, term->defaults.sgr.undline); break;
                case 9: param_set_if_lt(2, 16, term->defaults.sgr.strike);  break;
                default:
                    goto pvt_done;
            }
            break;
        case 1:     // ESC [=1~             - set current foreground/background as the default
            term->defaults.sgr.bg = term->sgr.bg;
            term->defaults.sgr.fg = term->sgr.fg;
            // TODO: should color rendition set by ESC[m carry over to the default??
            //       would need to run thru set_sgr() then convert back from VGA color to SGR color
            break;
        case 2:     // ESC [=2~             - blank screen now
            blank_screen(term);
            break;
        case 3:     // ESC [=3~             - unblank screen now
            unblank_screen(term);
            break;
        case 4:     // ESC [=4;<n>~         - set screen blank timeout interval to n minutes
            param_clamp_min(1, 0);
            param_clamp_max(1, MAX_BLANK_TIME);
            set_autoblank_timeout(term, term->csiparam[1]);
            break;
        case 5:     // ESC [=5~             - enable VGA character blink
            enable_blink(term);
            break;
        case 6:     // ESC [=6~             - disable VGA character blink
            disable_blink(term);
            break;
        case 7:     // ESC [=7;<n>~         - set BEL character frequency to n Hz
            param_clamp_min(1, MIN_BELL_HZ);
            param_clamp_max(1, MAX_BELL_HZ);
            term->defaults.bell.hz = term->csiparam[1];
            break;
        case 8:     // ESC [=8;<n>~         - set BEL character duration to n milliseconds
            param_clamp_min(1, MIN_BELL_MS);
            param_clamp_max(1, MAX_BELL_MS);
            term->defaults.bell.ms = term->csiparam[1];
            break;
        case 9:     // ESC [=9;<n>~         - switch to virtual terminal n
            if (term->csiparam[1] > 0 && term->csiparam[1] <= NR_TERMINAL) {
                term->disp.switch_to = term->csiparam[1];
            }
            break;
        default:
            goto pvt_done;  // invalid function
    }

pvt_done:
    term->disp.pending_wrap = false;
    term->seq_state = S_NORM;

pvt_next:
    return;
}

static int sgr_to_vga_color(int sgr_color)
{
    static const int SgrColorMap[16] =
    {
        VGA_BLACK,
        VGA_RED,
        VGA_GREEN,
        VGA_BROWN,
        VGA_BLUE,
        VGA_MAGENTA,
        VGA_CYAN,
        VGA_LIGHT_GRAY,
        VGA_DARK_GRAY,
        VGA_LIGHT_RED,
        VGA_LIGHT_GREEN,
        VGA_YELLOW,
        VGA_LIGHT_BLUE,
        VGA_LIGHT_MAGENTA,
        VGA_LIGHT_CYAN,
        VGA_WHITE,
    };

    return SgrColorMap[sgr_color & 0xF];
}

static void sgr_sequence(struct terminal *term, unsigned char c)
{
    //
    // Character Attributes via Set Graphics Rendition (SGR) control sequence.
    // CSIm
    //
    // https://www.man7.org/linux/man-pages/man4/console_codes.4.html
    // https://en.wikipedia.org/wiki/ANSI_escape_code
    //

    switch (c) {
        case 0:     // reset to defaults
            term->sgr._value = 0;
            term->sgr.bg = term->defaults.sgr.bg;
            term->sgr.fg = term->defaults.sgr.fg;
            break;
        case 1:     // set bright foreground (bold)
            term->sgr.bold = true;
            break;
        case 2:     // set faint (simulated with color)
            term->sgr.faint = true;
            break;
        case 3:     // set italic (simulated with color)
            term->sgr.italic = true;
            break;
        case 4:     // set underline (simulated with color)
        case 21:    // double underline not supported on VGA, alias as normal underline
            term->sgr.undline = true;
            break;
        case 5:     // set blink
        case 6:     // rapid blink not supported on VGA; alias as normal blink
            term->sgr.blink = true;
            break;
        case 7:     // set fg/bg color inversion
            term->sgr.invert = true;
            break;
        case 8:     // hide text
            term->sgr.conceal = true;
            break;
        case 9:     // strikethrough (simulated with color)
            term->sgr.strike = true;
            break;
        case 22:    // normal intensity (neither bright nor faint)
            term->sgr.bold = false;
            term->sgr.faint = false;
            break;
        case 23:    // disable italic
            term->sgr.italic = false;
            break;
        case 24:    // disable underline
            term->sgr.undline = false;
            break;
        case 25:    // disable blink
            term->sgr.blink = false;
            break;
        case 27:    // disable fg/bg inversion
            term->sgr.invert = false;
            break;
        case 28:    // reveal text
            term->sgr.conceal = false;
            break;
        case 29:    // disable strikethrough
            term->sgr.strike = false;
            break;
        default:
            // colors
            if (c >= 30 && c <= 37) {
                term->sgr.fg = (c - 30);
            }
            else if (c >= 40 && c <= 47) {
                term->sgr.bg = (c - 40);
            }
            else if (c == 39) {
                term->sgr.fg = term->defaults.sgr.fg;
            }
            else if (c == 49) {
                term->sgr.bg = term->defaults.sgr.bg;
            }
            else if (c >= 90 && c <= 97) {
                term->sgr.fg = (_SGR_BRIGHT_BIT | (c - 90));
            }
            else if (c >= 100 && c <= 107) {
                term->sgr.bg = (_SGR_BRIGHT_BIT | (c - 100));
            }
            break;
    }
}

static void cursor_style(struct terminal *term, int cursor)
{
    int max_height = (vga_crtc_read(VGA_CRTC_REG_MSL) & VGA_CRTC_FLD_MSL_MSL_MASK) - 1;
    if (max_height < 2) {
        max_height = 2;
    }

    switch (cursor) {
        case CURSOR_UNDERLINE:
            term->cursor.shape = ((max_height - 1) << 8) | (max_height - 2);
            break;
        case CURSOR_BLOCK:
            term->cursor.shape = ((max_height - 1) << 8) | 0;
            break;
        default:
            return; // invalid
    }

    if (is_current_terminal(term)) {
        apply_cursor_shape(term);
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
    struct _term_save_state *saved = &term->saved_state;
    static_assert(countof(saved->tabstops) == MAX_TABSTOP >> 3, "use MAX_TABSTOP");

    memcpy(saved->tabstops, term->tabstops, countof(saved->tabstops));
    saved->disp = term->disp;
    saved->sgr = term->sgr;
    saved->cursor = term->cursor;
}

static void restore_terminal(struct terminal *term)
{
    struct _term_save_state *saved = &term->saved_state;
    static_assert(countof(term->tabstops) == MAX_TABSTOP >> 3, "use MAX_TABSTOP");

    memcpy(term->tabstops, saved->tabstops, countof(term->tabstops));
    term->disp = saved->disp;
    term->sgr = saved->sgr;
    term->cursor = saved->cursor;

    if (is_current_terminal(term)) {
        apply_vga_state(term);
    }
}

static void save_cursor_pos(struct terminal *term)
{
    term->saved_state.cursor.x = term->cursor.x;
    term->saved_state.cursor.y = term->cursor.y;
}

static void restore_cursor_pos(struct terminal *term)
{
    term->cursor.x = term->saved_state.cursor.x;
    term->cursor.y = term->saved_state.cursor.y;

    if (is_current_terminal(term)) {
        apply_cursor_state(term);
    }
}

static void reverse_video(struct terminal *term)
{
    term->disp.reverse_video = !term->disp.reverse_video;

    // swap fg/bg of every cell in the scrolling ring buffer so that cells
    // scrolled into view later are also reversed
    struct vga_cell *fb = get_terminal_fb(term);
    for (int i = 0; i < VT_FB_WORDS; i++) {
        swap(fb[i].attr.bg, fb[i].attr.fg);
        if (term->sgr.blink) {
            fb[i].attr.blink = 1;
        }
    }
    swap(term->sgr.bg, term->sgr.fg);
}

static void unblank_screen(struct terminal *term)
{
    term->disp.blank_on = false;
    if (is_current_terminal(term)) {
        apply_display_blank(term);
    }
}

static void blank_screen(struct terminal *term)
{
    term->disp.blank_on = true;
    if (is_current_terminal(term)) {
        apply_display_blank(term);
    }
}

static void set_autoblank(struct terminal *term)
{
    term->autoblank_on = true;
    if (is_current_terminal(term)) {
        apply_display_blank(term);
    }
}

static void clear_autoblank(struct terminal *term)
{
    term->autoblank_on = false;
    if (is_current_terminal(term)) {
        apply_display_blank(term);
    }
}

static void set_autoblank_timeout(struct terminal *term, int min)
{
    if (min > MAX_BLANK_TIME) {
        min = MAX_BLANK_TIME;
    }
    else if (min < 0) {
        min = 0;
    }

    term->defaults.autoblank_interval = min;

    if (min == 0) {
        term->autoblank = false;
        term->next_autoblank = 0;
    }
    else {
        term->autoblank = true;
        rearm_autoblank(term);
    }

    if (term->autoblank_on) {
        clear_autoblank(term);
    }
}

static void rearm_autoblank(struct terminal *term)
{
    if (!term->autoblank) {
        return;
    }

    term->next_autoblank = get_uptime()
        + (term->defaults.autoblank_interval * 60 * 1000000000ULL);

    if (term->autoblank_on) {
        clear_autoblank(term);
    }
}

static void /*__isr*/ autoblank_timer(int irq, struct iregs *regs)
{
    assert(irq == IRQ_TIMER);
    (void) regs;

    uint64_t now = get_uptime();
    struct terminal *term = get_terminal(0);

    if (term->autoblank && !term->autoblank_on && now >= term->next_autoblank) {
        set_autoblank(term);   // TODO: use a DPC?
    }
}

static void enable_blink(struct terminal *term)
{
    term->disp.blink_on = true;
    if (is_current_terminal(term)) {
        apply_display_blink(term);
    }
}

static void disable_blink(struct terminal *term)
{
    term->disp.blink_on = false;
    if (is_current_terminal(term)) {
        apply_display_blink(term);
    }
}

static void show_cursor(struct terminal *term)
{
    term->cursor.hidden = false;
    if (is_current_terminal(term)) {
        apply_cursor_hidden(term);
    }
}

static void hide_cursor(struct terminal *term)
{
    term->cursor.hidden = true;
    if (is_current_terminal(term)) {
        apply_cursor_hidden(term);
    }
}

static void backspace(struct terminal *term)
{
    cursor_left(term, 1);
    term->disp.pending_wrap = false;
}

static void carriage_return(struct terminal *term)
{
    term->cursor.x = 0;
    term->disp.pending_wrap = false;
}

static void line_feed(struct terminal *term)
{
    if (++term->cursor.y >= term->rows) {
        scroll(term, 1);
        term->cursor.y--;
    }
    term->disp.pending_wrap = false;
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
    // advance to the next tab stop, stopping at the right margin
    while (term->cursor.x < term->cols - 1) {
        if (test_bit(term->tabstops, ++term->cursor.x)) {
            break;
        }
    }
    if (term->cursor.x >= term->cols - 1) {
        term->cursor.x = term->cols - 1;
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

    if (n_blank > n_visible) {
        n_blank = n_visible;
        n_kept = 0;
    }

    struct vga_cell *fb = get_terminal_fb(term);
    int last_row_start = VT_FB_WORDS - (VT_FB_WORDS % term->cols);
    int new_origin = (int) term->origin + (reverse ? -n_blank : n_blank);

    if (!reverse && (new_origin + n_visible > last_row_start)) {
        // forward scroll wraps past the usable buffer end;
        // move the kept rows to the start of the buffer
        memmove(fb, fb + new_origin, n_kept * sizeof(struct vga_cell));
        term->origin = 0;
    }
    else if (reverse && (new_origin < 0)) {
        // reverse scroll wraps before the buffer start;
        // move the kept rows to just before the end of the usable buffer.
        memmove(fb + (last_row_start - n_kept), fb + term->origin, n_kept * sizeof(struct vga_cell));
        term->origin = last_row_start - n_visible;
    }
    else {
        term->origin = (uint16_t) new_origin;
    }

    for (int i = 0; i < n_blank; i++) {
        int pos = (reverse) ? i : n_visible - n_blank + i;
        set_fb_cell(term, pos, ' ');
    }

    if (is_current_terminal(term)) {
        vga_set_scan_start((s_curr_page * VT_FB_WORDS) + term->origin);
    }

#else

    int n_blank = n * term->cols;
    int n_cells = (term->rows * term->cols) - n_blank;
    int n_bytes = n_cells * sizeof(struct vga_cell);

    struct vga_cell *fb = get_terminal_fb(term);
    void *src_end = &fb[n_blank];
    void *src = (reverse) ? fb : src_end;
    void *dst = (reverse) ? src_end : fb;
    memmove(dst, src, n_bytes);

    for (int i = 0; i < n_blank; i++) {
        int pos = (reverse) ? i : n_cells + i;
        set_fb_cell(term, pos, ' ');
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
            start = 0;
            count = area;
            break;
        default:
            return; // invalid
    }

    for (int i = 0; i < count; i++) {
        set_fb_cell(term, start + i, ' ');
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
            start = xy2pos(term, 0, term->cursor.y);
            count = term->cols;
            break;
        default:
            return; // invalid
    }

    for (int i = 0; i < count; i++) {
        set_fb_cell(term, start + i, ' ');
    }
}

static void insert_tabstop(struct terminal *term)
{
    if (term->cursor.x < MAX_TABSTOP) {
        set_bit(term->tabstops, term->cursor.x);
    }
}

static void clear_tabstop(struct terminal *term, int mode)
{
    switch (mode) {
        case 0: // ESC [0g  - erase tabstop at current column
            if (term->cursor.x < MAX_TABSTOP) {
                clear_bit(term->tabstops, term->cursor.x);
            }
            break;
        case 2: // ESC [2g  - restore default tabstops (non-standard)
            default_tabstops(term);
            break;
        case 3: // ESC [3g  - erase all tabstops
            memset(term->tabstops, 0, countof(term->tabstops));
            break;
    }
}

static void default_tabstops(struct terminal *term)
{
    for (int i = 0; i < MAX_TABSTOP; i++) {
        if (((i + 1) % TABSTOP_WIDTH) == 0) {
            set_bit(term->tabstops, i);
        }
        else {
            clear_bit(term->tabstops, i);
        }
    }
}

static void cursor_up(struct terminal *term, int n)
{
    if (n < 0) {
        return;
    }
    if (n >= term->cursor.y) {
        term->cursor.y = 0;
    }
    else {
        term->cursor.y -= n;
    }
}

static void cursor_down(struct terminal *term, int n)
{
    if (n < 0) {
        return;
    }

    if (n >= term->rows - 1 - term->cursor.y) {
        term->cursor.y = term->rows - 1;
    }
    else {
        term->cursor.y += n;
    }
}

static void cursor_left(struct terminal *term, int n)
{
    if (n < 0) {
        return;
    }
    if (n >= term->cursor.x) {
        term->cursor.x = 0;
    }
    else {
        term->cursor.x -= n;
    }
}

static void cursor_right(struct terminal *term, int n)
{
    if (n < 0) {
        return;
    }

    if (n >= term->cols - 1 - term->cursor.x) {
        term->cursor.x = term->cols - 1;
    }
    else {
        term->cursor.x += n;
    }
}

static uint16_t xy2pos(const struct terminal *term, uint16_t x, uint16_t y)
{
    x = min(x, term->cols-1);
    y = min(y, term->rows-1);

    return y * term->cols + x;
}

static void pos2xy(struct terminal *term, uint16_t pos)
{
    pos = (pos - term->origin + VT_FB_WORDS) % VT_FB_WORDS;
    term->cursor.x = pos % term->cols;
    term->cursor.y = pos / term->cols;
}

static struct vga_cell * get_terminal_fb(struct terminal *term)
{
    return is_current_terminal(term) ? get_curr_hwfb_page() : term->backbuf;
}

static void set_sgr(const struct terminal *term, struct vga_cell *cell, struct _sgr_attr *sgr)
{
    cell->attr.bg = sgr_to_vga_color(sgr->bg);
    cell->attr.fg = sgr_to_vga_color(sgr->fg);

    if (sgr->bold) {
        cell->attr.bold = 1;
    }
    if (sgr->faint) {
        cell->attr.fg = sgr_to_vga_color(term->defaults.sgr.faint);
        cell->attr.bg = VGA_BLACK;
    }
    if (sgr->undline) {
        cell->attr.fg = sgr_to_vga_color(term->defaults.sgr.undline);
    }
    if (sgr->italic) {
        cell->attr.fg = sgr_to_vga_color(term->defaults.sgr.italic);
    }
    if (sgr->strike) {
        cell->attr.fg = sgr_to_vga_color(term->defaults.sgr.strike);
    }
    if (sgr->invert) {
        swap(cell->attr.bg, cell->attr.fg);
    }
    if (sgr->conceal) {
        cell->attr.fg = cell->attr.bg;
    }
    if (sgr->blink) {
        cell->attr.blink = 1;
    }
}

static void set_fb_cell(struct terminal *term, uint16_t pos, char c)
{
    struct vga_cell cell, *fb;

    fb = get_terminal_fb(term);
    pos = (term->origin + pos) % VT_FB_WORDS;
    cell = fb[pos];

    if (c != '\0') {
        cell.ch = c;
    }

    set_sgr(term, &cell, &term->sgr);
    fb[pos] = cell;
}

static void apply_display_blink(const struct terminal *term)
{
    vga_enable_blink(term->disp.blink_on);
}

static void apply_display_blank(const struct terminal *term)
{
    vga_set_screen_blank(term->disp.blank_on || term->autoblank_on);
}

static void apply_display_state(const struct terminal *term)
{
    apply_display_blink(term);
    apply_display_blank(term);
}

static void apply_cursor_hidden(const struct terminal *term)
{
    vga_enable_cursor(!term->cursor.hidden);
}

static void apply_cursor_pos(const struct terminal *term)
{
    uint16_t pos = (s_curr_page * VT_FB_WORDS) + term->origin + xy2pos(term, term->cursor.x, term->cursor.y);
    vga_set_cursor_pos(pos);
}

static void apply_cursor_shape(const struct terminal *term)
{
    vga_set_cursor_shape(term->cursor.shape);
}

static void apply_cursor_state(const struct terminal *term)
{
    apply_cursor_hidden(term);
    apply_cursor_shape(term);
    apply_cursor_pos(term);
}

static void apply_vga_state(const struct terminal *term)
{
    apply_display_state(term);
    apply_cursor_state(term);
#if VSYNC
    vga_wait_for_vsync();
#endif
    vga_set_scan_start((s_curr_page * VT_FB_WORDS) + term->origin);
}
