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
#include <kernel/char.h>
#include <kernel/console.h>
#include <kernel/irq.h>
#include <kernel/kernel.h>
#include <kernel/ohwes.h>
#include <kernel/tty.h>
#include <kernel/terminal.h>
#include <kernel/vga.h>

// initialization
extern void init_vga(void);
static void initialize_terminal(int num, struct terminal *term);

// screen positioning
static uint16_t xy2pos(const struct terminal *term, uint16_t x, uint16_t y);
static void pos2xy(struct terminal *term, uint16_t pos);

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// TTY device implementation

static int terminal_tty_open(struct tty *);
static int terminal_tty_close(struct tty *);
static int terminal_tty_ioctl(struct tty *, int op, void *arg);
static int terminal_tty_write(struct tty *, const char *buf, size_t count);
static void terminal_tty_write_char(struct tty *, char c);
static size_t terminal_tty_write_room(struct tty *);

static struct tty_driver terminal_driver = {
    .name = "tty",
    .major = TTY_MAJOR,
    .minor_start = TTY_MIN,
    .count = NR_TERMINAL,
    .open = terminal_tty_open,
    .close = terminal_tty_close,
    .ioctl = terminal_tty_ioctl,
    .write = terminal_tty_write,
    ./* in the */write_room/* with black curtains*/ = terminal_tty_write_room,
    .flush = NULL,
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
        return -EBUSY;  // TODO: return 0?
    }

    term->tty = tty;
    return 0;
}

static int terminal_tty_close(struct tty *tty)
{
    struct terminal *term;

    int ret = tty_get_terminal(tty, &term);
    if (ret < 0) {
        return ret;
    }

    term->tty = NULL;
    return 0;
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

    ret = terminal_write(term, buf, count);
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
    return 4096;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// console implementation

static dev_t vt_console_device(struct console *cons)
{
    return __mkttydev((cons->index) ? cons->index : current_terminal());
}

static void vt_console_setup(struct console *cons)
{
    struct vga_fb_info fb_info;

    init_vga(); // ok to call more than once
    vga_get_fb_info(&fb_info);

    struct terminal *term = get_terminal(cons->index);
    if (!term->initialized) {
        int num = _DEV_MIN(cons->device(cons));
        initialize_terminal(num, term);
        if (num == 1) {
            term->framebuf = (void *) KERNEL_ADDR(fb_info.framebuf);
            pos2xy(term, vga_get_cursor_pos());
        }
    }
}

static int vt_console_write(struct console *cons, const char *buf, size_t count)
{
    struct terminal *term;
    const char *p;

    term = get_terminal(cons->index);

    p = buf;
    while (*p != '\0' && (p - buf) < count) {
        if (*p == '\n') {
            terminal_putchar(term, '\r');
        }
        terminal_putchar(term, *p);
        p++;
    }

    return (p - buf);
}

static int vt_console_getc(struct console *cons)
{
    return kb_getc();
}

struct console vt_console =
{
    .name = "tty",
    .index = VT_CONSOLE_NUM,
    .flags = 0,
    .device = vt_console_device,
    .setup = vt_console_setup,
    .write = vt_console_write,
    .getc = vt_console_getc
};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// virtual terminal implementation

#define is_current(term)        ((term)->number == current_terminal())

struct terminal g_terminals[NR_TERMINAL];
int g_currterm = 1;

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
static void enable_blink(const struct terminal *term);  // ESC 3 / ESC 4
static void enable_cursor(const struct terminal *term); // ESC 5 / ESC 6
static void set_cursor_pos(const struct terminal *term);// ESC [ <n>;<m>H
static void set_cursor_shape(const struct terminal *term);
static void update_vga_state(const struct terminal *term);

// frame buffer
static void set_fb_char(struct terminal *term, uint16_t pos, char c);
static void set_fb_attr(struct terminal *term, uint16_t pos, struct _char_attr attr);

// ----------------------------------------------------------------------------
// initialization

void init_terminal(void)
{
    struct vga_fb_info fb_info;

    init_vga(); // ok to call more than once
    vga_get_fb_info(&fb_info);
    kprint("vga: frame buffer is %d pages at %08X\n",
        fb_info.size_pages, fb_info.framebuf);

    // make sure we have enough memory for the configured number of terminals
    if (fb_info.size_pages - FB_SIZE_PAGES < NR_TERMINAL * FB_SIZE_PAGES) {
        panic("not enough video memory available for %d terminals at "
            "%d frame buffer\npages each! See config.h.",
            NR_TERMINAL, FB_SIZE_PAGES);
    }

    // register the terminal TTY driver
    if (tty_register_driver(&terminal_driver)) {
        panic("unable to register terminal driver!");
    }

    // initialize virtual terminals
    for (int i = 1; i <= NR_TERMINAL; i++) {
        struct terminal *term = get_terminal(i);
        if (term->initialized) {
            // terminal already initialized if console was registered early
            continue;
        }
        initialize_terminal(i, term);
        erase(term, ERASE_ALL);
    }

    // restore boot terminal state
    get_terminal(1)->framebuf = (void *) KERNEL_ADDR(fb_info.framebuf);
    pos2xy(get_terminal(1), vga_get_cursor_pos());

    // do a proper 'switch' to the initial virtual terminal
    int ret = switch_terminal(DEFAULT_VT);
    if (ret != 0) {
        panic("unable to switch to terminal %d!", DEFAULT_VT);
    }

    // create a restore point
    save_terminal(get_terminal(DEFAULT_VT));

    // enable blink, show cursor
    terminal_print(get_terminal(DEFAULT_VT), "\e4\e6");

    // register the virtual terminal console
    register_console(&vt_console);

#if PRINT_LOGO
    kprint( // let's print a bird with a blinking eye lol
    "\e[1;37m                                                                           \n\
                                                     ,::::.._                           \n\
                                                  ,':::::::::.                          \n\
                                              _,-'`:::,::(\e[5;31mo\e[25;37m)::`-,.._   \n\
                                           _.', ', `:::::::::;'-..__`.                  \n\
                                      _.-'' ' ,' ,' ,\\:::,'::-`'''                     \n\
                                  _.-'' , ' , ,'  ' ,' `:::/                            \n\
                            _..-'' , ' , ' ,' , ,' ',' '/::                             \n\
                    _...:::'`-..'_, ' , ,'  , ' ,'' , ,'::|                             \n\
                 _`.:::::,':::::,'::`-:..'_',_'_,'..-'::,'|                             \n\
         _..-:::'::,':::::::,':::,':,'::,':::,'::::::,':::;                             \n\
           `':,'::::::,:,':::::::::::::::::':::,'::_:::,'/                              \n\
           __..:'::,':::::::--''' `-:,':,':::'::-' ,':::/                               \n\
      _.::::::,:::.-''-`-`..'_,'. ,',  , ' , ,'  ', `','                                \n\
    ,::SSt:''''`                 \\:. . ,' '  ,',' '_,'                                 \n\
                                  ``::._,'_'_,',.-'                                     \n\
                                      \\\\ \\\\                                         \n\
                                       \\\\_\\\\                                        \n\
                                        \\\\`-`.-'_                                     \n\
                                     .`-.\\\\__`. ``                                    \n\
                                        ``-.-._                                         \n\
                                            `                                           \n\
    \e[0m\n"); //https://ascii.co.uk/art/raven
#endif
}

static void initialize_terminal(int num, struct terminal *term)
{
    if (term->initialized) {
        return;
    }

    terminal_defaults(term);
    term->number = num;
    term->framebuf = get_terminal_fb(num);
    term->initialized = true;
}

// ----------------------------------------------------------------------------
// public functions

void terminal_defaults(struct terminal *term)
{
    term->state = S_NORM;
    term->cols = vga_get_cols();
    term->rows = vga_get_rows();
    for (int i = 0; i < MAX_TABSTOP; i++) {
        term->tabstops[i] = (((i + 1) % TABSTOP_WIDTH) == 0);
    }
    memset(term->csiparam, -1, sizeof(term->csiparam));
    term->paramidx = 0;
    term->blink_on = false;
    term->need_wrap = false;
    term->attr.bg = VGA_BLACK;
    term->attr.fg = VGA_WHITE;
    term->attr.bright = false;
    term->attr.faint = false;
    term->attr.italic = false;
    term->attr.underline = false;
    term->attr.blink = false;
    term->attr.invert = false;
    term->cursor.x = 0;
    term->cursor.y = 0;
    term->cursor.shape = vga_get_cursor_shape();    // TODO: default shape
    term->cursor.hidden = false;
    term->csi_defaults.attr = term->attr;
    term->csi_defaults.cursor = term->cursor;
    save_terminal(term);
}

extern int tty_open_internal(struct tty *tty);

int switch_terminal(int num)
{
    if (num <= 0 || num > NR_TERMINAL) {
        return -EINVAL;
    }

    pde_t *pgdir;

    uint32_t flags;
    cli_save(flags);

    struct vga_fb_info fb_info;
    struct terminal *curr = get_terminal(0);
    struct terminal *next = get_terminal(num);
    struct tty *tty = NULL;

    if (get_tty(__mkdev(TTY_MAJOR, num), &tty)) {
        panic("tty%d not found", num);
    }
    if (tty_open_internal(tty)) {
        panic("could not switch terminals -- unable to open tty%d", num);
    }

    vga_get_fb_info(&fb_info);
    curr->framebuf = get_terminal_fb(curr->number);
    next->framebuf = get_terminal_fb(next->number);

    pgdir = (pde_t *) get_pgdir();

#if HIGHER_GROUND
    // enable kernel identity mapping so we can operate on page tables
    pde_t *ident_pde = &pgdir[0];
    *ident_pde = __mkpde(KERNEL_PGTBL, _PAGE_RW);
#endif

    // identity map old frame buffer, so it will write to back buffer
    for (int i = 0; i < FB_SIZE_PAGES; i++) {
        uint32_t fb_page = (uint32_t) curr->framebuf + (i << PAGE_SHIFT);
        pte_t *pte = pte_offset(pgdir, fb_page);
        *pte = __mkpte(PHYSICAL_ADDR(fb_page), _PAGE_RW);
    }
    flush_tlb();

    // swap buffers
    memcpy(curr->framebuf, (void *) fb_info.framebuf, FB_SIZE);
    memcpy((void *) fb_info.framebuf, next->framebuf, FB_SIZE);
    curr = next;

    // map new frame buffer to VGA
    for (int i = 0; i < FB_SIZE_PAGES; i++) {
        uint32_t fb_page = (uint32_t) curr->framebuf + (i << PAGE_SHIFT);
        uint32_t vga_page = fb_info.framebuf + (i << PAGE_SHIFT);
        pte_t *pte = pte_offset(pgdir, fb_page);
        *pte = __mkpte(PHYSICAL_ADDR(vga_page), _PAGE_RW);
    }

#if HIGHER_GROUND
    pde_clear(ident_pde);
#endif

    flush_tlb();

    update_vga_state(curr);
    g_currterm = curr->number;

    restore_flags(flags);
    return 0;
}

int current_terminal(void)
{
    if (g_currterm <= 0 || g_currterm > NR_TERMINAL) {
        panic("g_currterm is somehow %d!", g_currterm);
    }
    return g_currterm;
}

struct terminal * get_terminal(int num)
{
    if (num < 0 || num > NR_TERMINAL) {
        panic("attempt to get nonexistant terminal %d!", num);
    }
    if (num == 0) {
        num = current_terminal();
    }
    assert(num > 0);

    struct terminal *term = &g_terminals[num - 1];
    if (term->initialized) {
        assert(term->number == num);
    }

    return term;
}

void * get_terminal_fb(int num)
{
    char *fb;

    if (num < 0 || num > NR_TERMINAL) {
        panic("attempt to get nonexistant terminal %d frame buffer!", num);
    }
    if (num == 0) {
        num = current_terminal();
    }
    assert(num > 0);

    fb = (char *) get_vga_fb();
    fb += (num * FB_SIZE_PAGES) << PAGE_SHIFT;

    return fb;
}

void * get_vga_fb(void)
{
    struct vga_fb_info fb_info;
    vga_get_fb_info(&fb_info);

    return (void *) KERNEL_ADDR(fb_info.framebuf);
}

void terminal_save(struct terminal *term, struct terminal_save_state *save)
{
    memcpy(save->tabstops, term->tabstops, MAX_TABSTOP);
    save->blink_on = term->blink_on;
    save->attr = term->attr._value;
    save->cursor = term->cursor._value;
}

void terminal_restore(struct terminal *term, struct terminal_save_state *save)
{
    memcpy(term->tabstops, save->tabstops, MAX_TABSTOP);
    term->blink_on = save->blink_on;
    term->attr._value = save->attr;
    term->cursor._value = save->cursor;

    if (is_current(term)) {
        update_vga_state(term);
    }
}

int terminal_print(struct terminal *term, const char *buf)
{
    const char *p;

    if (!term || !buf) {
        return -EINVAL;
    }

    p = buf;
    while (*p != '\0' && (p - buf) < MAX_PRINTBUF) {
        p += terminal_putchar(term, *p);
    }

    return (p - buf);
}

int terminal_write(struct terminal *term, const char *buf, size_t count)
{
    const char *p;

    if (!term || !buf) {
        return -EINVAL;
    }

    p = buf;
    while (p < buf + count) {
        p += terminal_putchar(term, *p);
    }

    return count;
}

int terminal_putchar(struct terminal *term, char c)
{
    bool update_char = false;
    bool update_attr = false;
    bool update_cursor_pos = true;
    uint16_t char_pos;

    // prevent reentrancy
    if (test_and_set_bit(&term->printing, 0)) {
        return 0;   // TODO: do we just drop the char?
                    // we should buffer the char then flush it at the end
    }

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
            beep(BELL_FREQ, BELL_TIME);         // TODO: ioctl to control beep tone/time
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

            update_char = true;
            update_attr = true;

            // handle deferred wrap
            if (term->need_wrap) {
                carriage_return(term);
                line_feed(term);
            }

            // determine character position
            char_pos = xy2pos(term, term->cursor.x, term->cursor.y);

            // advance cursor
            term->cursor.x++;
            if (term->cursor.x >= term->cols) {
                // if the cursor is at the end of the line, prevent
                // the display from scrolling one line (wrapping) until
                // the next character is received so we aren't left with
                // an unnecessary blank line
                term->cursor.x--;
                term->need_wrap = true;
                update_cursor_pos = false;
            }
            break;
    }

write_vga:
    if (update_char) {
        set_fb_char(term, char_pos, c);
    }
    if (update_attr) {
        if (term->attr.bright && term->attr.faint) {
            term->attr.bright = false;      // faint overrides bright
        }
        set_fb_attr(term, char_pos, term->attr);
    }
    if (update_cursor_pos && is_current(term)) {
        set_cursor_pos(term);
    }

    // TODO: flush buffered chars?

done:
    clear_bit(&term->printing, 0);
    return 1;
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
            if (is_current(term)) {
                enable_blink(term);
            }
            break;
        case '4':       // ESC 4    enable blink
            term->blink_on = true;
            if (is_current(term)) {
                enable_blink(term);
            }
            break;
        case '5':       // ESC 5    hide cursor
            term->cursor.hidden = true;
            if (is_current(term)) {
                enable_cursor(term);
            }
            break;
        case '6':       // ESC 6    show cursor
            term->cursor.hidden = false;
            if (is_current(term)) {
                enable_cursor(term);
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

    term->need_wrap = false;
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

    #define param_minimum(index,value)          \
    do {                                        \
        if (term->csiparam[index] < (value)) {  \
            term->csiparam[index] = (value);    \
        }                                       \
    } while (0)

    #define param_maximum(index,value)          \
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
            param_minimum(0, 1);
            cursor_up(term, term->csiparam[0]);
            goto csi_done;
        case 'B':       // CSI n B  - CUD - move cursor down n rows
            param_minimum(0, 1);
            cursor_down(term, term->csiparam[0]);
            goto csi_done;
        case 'C':       // CSI n C  - CUF - move cursor right (forward) n columns
            param_minimum(0, 1);
            cursor_right(term, term->csiparam[0]);
            goto csi_done;
        case 'D':       // CSI n D  - CUB - move cursor left (back) n columns
            param_minimum(0, 1);
            cursor_left(term, term->csiparam[0]);
            goto csi_done;
        case 'E':       // CSI n E  - CNL - move cursor to beginning of line, n rows down
            param_minimum(0, 1);
            term->cursor.x = 0;
            cursor_down(term, term->csiparam[0]);
            goto csi_done;
        case 'F':       // CSI n F  - CPL - move cursor to beginning of line, n rows up
            param_minimum(0, 1);
            term->cursor.x = 0;
            cursor_up(term, term->csiparam[0]);
            goto csi_done;
        case 'G':       // CSI n G  - CHA - move cursor to column n
            param_minimum(0, 1);
            param_maximum(0, term->cols);
            term->cursor.x = term->csiparam[0] - 1;
            goto csi_done;
        case 'H':       // CSI n ; m H - CUP - move cursor to row n, column m
            param_minimum(0, 1);
            param_minimum(1, 1);
            param_maximum(0, term->rows);
            param_maximum(1, term->cols);
            term->cursor.y = term->csiparam[0] - 1;
            term->cursor.x = term->csiparam[1] - 1;
            goto csi_done;
        case 'J':       // CSI n J  - ED - erase in display (n = mode)
            param_minimum(0, 0);
            erase(term, term->csiparam[0]);
            goto csi_done;
        case 'K':       // CSI n K  - EL- erase in line (n = mode)
            param_minimum(0, 0);
            erase_line(term, term->csiparam[0]);
            goto csi_done;
        case 'S':       // CSI n S  - SU - scroll n lines
            param_minimum(0, 1);
            scroll(term, term->csiparam[0]);
            goto csi_done;
        case 'T':       // CSI n T  - ST - reverse scroll n lines
            param_minimum(0, 1);
            scroll(term, -term->csiparam[0]);     // note the negative for reverse!
            goto csi_done;
        case 'm':       // CSI n m  - SGR - set graphics attribute
            for (int i = 0; i <= term->paramidx; i++){
                param_minimum(i, 0);
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

csi_done:   // CSI processing done
    term->need_wrap = false;
    term->state = S_NORM;

csi_next:   // we need more CSI characters; do not alter terminal state
    return;

    #undef param_minimum
    #undef param_maximum
}

static void csi_m(struct terminal *term, char p)
{
    static const char CSI_COLORS[8] =
    {
        // TODO: configure via ioctl
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
        case 1:     // set bright (bold)
            term->attr.bright = true;
            break;
        case 2:     // set faint (simulated with color)
            term->attr.faint = true;
            break;
        case 3:     // set italic (simulated with color)
            term->attr.italic = true;
            break;
        case 4:     // set underline (simulated with color)
            term->attr.underline = true;
            break;
        case 5:     // set blink
            term->attr.blink = true;
            break;
        case 7:     // set fg/bg color inversion
            term->attr.invert = true;
            break;
        case 22:    // normal intensity (neither bright nor faint)
            term->attr.bright = false;
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
        default:
            // colors
            if (p >= 30 && p <= 37) term->attr.fg = CSI_COLORS[p - 30];
            if (p >= 40 && p <= 47) term->attr.bg = CSI_COLORS[p - 40];
            if (p == 39) term->attr.fg = term->csi_defaults.attr.fg;
            if (p == 49) term->attr.bg = term->csi_defaults.attr.bg;
            if (p >= 90 && p <= 97) {
                term->attr.fg = CSI_COLORS[p - 90];
                term->attr.bright = 1;
            }
            if (p >= 100 && p <= 107) {
                term->attr.bg = CSI_COLORS[p - 100];
                term->attr.bright = !term->attr.blink;  // blink overrides bright
            }
            break;
    }
}

static void reset_terminal(struct terminal *term)
{
    terminal_defaults(term);
    erase(term, ERASE_ALL);
    if (is_current(term)) {
        update_vga_state(term);
    }
}

static void save_terminal(struct terminal *term)
{
    terminal_save(term, &term->saved_state);
}

static void restore_terminal(struct terminal *term)
{
    terminal_restore(term, &term->saved_state);
}

static void save_cursor(struct terminal *term)
{
    term->saved_state.cursor = term->cursor._value;
}

static void restore_cursor(struct terminal *term)
{
    term->cursor._value = term->saved_state.cursor;
    if (is_current(term)) {
        update_vga_state(term);
    }
}

static void backspace(struct terminal *term)
{
    cursor_left(term, 1);
    term->need_wrap = false;
}

static void carriage_return(struct terminal *term)
{
    term->cursor.x = 0;
    term->need_wrap = false;
}

static void line_feed(struct terminal *term)
{
    if (++term->cursor.y >= term->rows) {
        scroll(term, 1);
        term->cursor.y--;
    }
    term->need_wrap = false;
}

static void reverse_linefeed(struct terminal *term)
{
    if (--term->cursor.y < 0) {     // TODO: fix so it doesn't wrap
        scroll(term, -1);
        term->cursor.y++;
    }
    term->need_wrap = false;
}

static void tab(struct terminal *term)
{
    while (term->cursor.x < term->cols) {
        if (term->tabstops[++term->cursor.x]) {
            break;
        }
    }

    if (term->cursor.x >= term->cols) {
        term->cursor.x = term->cols - 1;
    }
}

static void scroll(struct terminal *term, int n)   // n < 0 is reverse scroll
{
    int n_cells;
    int n_blank;
    int n_bytes;
    bool reverse;
    void *src;
    void *src_end;
    void *dst;
    int i;

    reverse = (n < 0);
    if (reverse) {
        n = -n;
    }
    if (n > term->rows) {
        n = term->rows;
    }
    if (n == 0) {
        return;
    }

    n_blank = n * term->cols;
    n_cells = (term->rows * term->cols) - n_blank;
    n_bytes = n_cells * sizeof(struct vga_cell);

    src_end = &((struct vga_cell *) term->framebuf)[n_blank];
    src = (reverse) ? term->framebuf : src_end;
    dst = (reverse) ? src_end : term->framebuf;
    memmove(dst, src, n_bytes);

    for (i = 0; i < n_blank; i++) {
        int pos = (reverse) ? i : n_cells + i;
        set_fb_char(term, pos, ' ');
        set_fb_attr(term, pos, term->attr);
    }
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
    return y * term->cols + x;
}

static void pos2xy(struct terminal *term, uint16_t pos)
{
    term->cursor.x = pos % term->cols;
    term->cursor.y = pos / term->cols;
}

static void set_fb_char(struct terminal *term, uint16_t pos, char c)
{
    ((struct vga_cell *) term->framebuf)[pos].ch = c;
}

static void set_fb_attr(struct terminal *term, uint16_t pos, struct _char_attr attr)
{
    struct vga_attr *vga_attr;
    vga_attr = &((struct vga_cell *) term->framebuf)[pos].attr;

    vga_attr->bg = attr.bg;
    vga_attr->fg = attr.fg;

    if (attr.bright) {
        vga_attr->bright = 1;
    }
    if (attr.faint) {
        vga_attr->color_fg = VGA_BLACK;  // simulate faintness with dark gray   TODO: ioctl configure
        vga_attr->bright = 1;
    }
    if (attr.underline) {
        vga_attr->color_fg = VGA_CYAN;   // simulate underline with cyan
        vga_attr->bright = attr.bright;
    }
    if (attr.italic) {
        vga_attr->color_fg = VGA_GREEN;  // simulate italics with green
        vga_attr->bright = attr.bright;
    }
    if (attr.blink) {
        vga_attr->blink = 1;
    }
    if (attr.invert) {
        swap(vga_attr->color_bg, vga_attr->color_fg);
    }
}

static void enable_blink(const struct terminal *term)
{
    vga_enable_blink(term->blink_on);
}

static void enable_cursor(const struct terminal *term)
{
    vga_enable_cursor(!term->cursor.hidden);
}

static void set_cursor_pos(const struct terminal *term)
{
    uint16_t pos;

    pos = xy2pos(term, term->cursor.x, term->cursor.y);
    vga_set_cursor_pos(pos);
}

static void set_cursor_shape(const struct terminal *term)
{
    vga_set_cursor_shape(term->cursor.shape);
}

static void update_vga_state(const struct terminal *term)
{
    enable_blink(term);
    enable_cursor(term);
    set_cursor_shape(term);
    set_cursor_pos(term);
}
