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
#include <i386/interrupt.h>
#include <i386/io.h>
#include <i386/paging.h>
#include <i386/x86.h>
#include <kernel/ohwes.h>
#include <kernel/chdev.h>
#include <kernel/kernel.h>
#include <kernel/tty.h>
#include <kernel/terminal.h>
#include <kernel/vga.h>

// TODO: need to make a distinction between a 'console':
//   a character device that can receive input, transmit output and is
//   physically connected to the computer (e.g. keyboard+vga or a serial device)
// and a 'vga terminal':
//   a (PS/2) keyboard input and VGA output device that are connected to the
//   system as a console

#define is_syscon(cons)         ((cons)->number == SYSTEM_CONSOLE)
#define is_current(cons)        ((cons) == current_console())

__data_segment struct console g_consoles[NR_CONSOLE] = { };

static int console_tty_open(struct tty *);
static int console_tty_close(struct tty *);
static int console_tty_ioctl(struct tty *, int op, void *arg);
static int console_tty_write(struct tty *, const char *buf, size_t count);
static void console_tty_write_char(struct tty *, char c);
static size_t console_tty_write_room(struct tty *);

static struct tty_driver console_driver = {
    .name = "tty",
    .major = TTY_MAJOR,
    .minor_start = CONSOLE_MIN,
    .count = NR_CONSOLE,
    .open = console_tty_open,
    .close = console_tty_close,
    .ioctl = console_tty_ioctl,
    .write = console_tty_write,
    ./* in the */write_room/* with black curtains*/ = console_tty_write_room,
    .flush = NULL,
};

static int tty_get_console(struct tty *tty, struct console **console)
{
    if (!tty || !console) {
        return -EINVAL;
    }

    if (_DEV_MAJ(tty->device) != TTY_MAJOR) {
        return -ENODEV; // not a TTY device
    }

    int index = _DEV_MIN(tty->device);
    if (index < CONSOLE_MIN || index > CONSOLE_MAX) {
        return -ENXIO;  // TTY device is not a console
    }

    *console = get_console(index);
    return 0;
}

static int console_tty_open(struct tty *tty)
{
    struct console *cons;

    int ret = tty_get_console(tty, &cons);
    if (ret < 0) {
        return ret;
    }

    if (cons->open) {
        assert(cons->tty);
        return -EBUSY;  // console already open
    }

    cons->tty = tty;
    cons->open = true;
    return 0;
}

static int console_tty_close(struct tty *tty)
{
    struct console *cons;

    int ret = tty_get_console(tty, &cons);
    if (ret < 0) {
        return ret;
    }

    cons->tty = NULL;
    cons->open = false;
    return 0;
}

static int console_tty_write(struct tty *tty, const char *buf, size_t count)
{
    struct console *cons;

    if (!buf) {
        return -EINVAL;
    }

    int ret = tty_get_console(tty, &cons);
    if (ret < 0) {
        return ret;
    }

    ret = console_write(cons, buf, count);
    if (tty->driver.flush) {
        tty->driver.flush(tty);
    }

    return ret;
}

static int console_tty_ioctl(struct tty *tty, int op, void *arg)
{
    return -ENOTTY;
}

static size_t console_tty_write_room(struct tty *tty)
{
    // we can write the frame buffer forever...
    // return something sufficiently large to satisfy ldisc logic
    return 4096;
}

// ----------------------------------------------------------------------------

// TODO: most of this should be in a file named terminal.c - because it emulates
// a VGA terminal screen

enum console_state {
    S_NORM,
    S_ESC,
    S_CSI
};

// console state
static void reset(struct console *cons);                // ESC c
static void save_console(struct console *cons);         // ESC 7
static void restore_console(struct console *cons);      // ESC 8
static void cursor_save(struct console *cons);          // ESC [s
static void cursor_restore(struct console *cons);       // ESC [u

// character handling
static void esc(struct console *cons, char c);          // ^[ (ESC)
static void csi(struct console *cons, char c);          // ESC [
static void csi_m(struct console *cons, char p);        // ESC [<params>m
static void backspace(struct console *cons);            // ^H
static void tab(struct console *cons);                  // ^I
static void line_feed(struct console *cons);            // ^J
static void reverse_linefeed(struct console *cons);     // ESC M
static void carriage_return(struct console *cons);      // ^M
static void scroll(struct console *cons, int n);        // ESC [<n>S / ESC [<n>T
static void erase(struct console *cons, int mode);      // ESC [<n>J
static void erase_line(struct console *cons, int mode); // ESC [<n>K
static void cursor_up(struct console *cons, int n);     // ESC [<n>A
static void cursor_down(struct console *cons, int n);   // ESC [<n>B
static void cursor_right(struct console *cons, int n);  // ESC [<n>C
static void cursor_left(struct console *cons, int n);   // ESC [<n>D

// VGA programming
static void vga_blink_enable(const struct console *cons); // ESC 3 / ESC 4
static void vga_cursor_enable(const struct console *cons);// ESC 5 / ESC 6
static void vga_set_cursor_pos(const struct console *cons);   // ESC [ <n>;<m>H
static void vga_set_cursor_shape(const struct console *cons);

// frame buffer
static void set_fb_char(struct console *cons, uint16_t pos, char c);
static void set_fb_attr(struct console *cons, uint16_t pos, struct _char_attr attr);

// screen positioning
static uint16_t xy2pos(uint16_t ncols, uint16_t x, uint16_t y);

// ----------------------------------------------------------------------------
// initialization

void init_console(void)
{
    // make sure we have enough memory for the configured number of consoles
    if (g_vga->fb_info.size_pages - FB_SIZE_PAGES < NR_CONSOLE * FB_SIZE_PAGES) {
        panic("not enough video memory available for %d consoles! See config.h.", NR_CONSOLE);
    }

    // register the console driver
    if (tty_register_driver(&console_driver)) {
        panic("unable to register console driver!");
    }

    // initialize virtual consoles
    for (int i = 1; i <= NR_CONSOLE; i++) {
        struct console *cons = get_console(i);
        cons->framebuf = get_console_fb(i);
        cons->number = i;
        console_defaults(cons);
        erase(cons, 2);
    }

    // restore console state from boot
    struct console *cons = get_console(SYSTEM_CONSOLE);
    cons->cursor.x = g_boot->cursor_col;
    cons->cursor.y = g_boot->cursor_row;
    cons->cursor.shape = g_vga->orig_cursor_shape;
    cons->framebuf = (void *) g_vga->fb_info.framebuf;

    // do a proper 'switch' to the initial virtual console
    int ret = switch_console(SYSTEM_CONSOLE);
    if (ret != 0) {
        panic("failed to initialize system console!");
    }
    assert(cons == get_console(SYSTEM_CONSOLE));

    // create a restore point
    save_console(cons);

    // enable blink, show cursor
    kprint("\e4\e6");

#if PRINT_LOGO
    kprint( // safe to print now, so let's print a bird with a blinking eye lol
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

// ----------------------------------------------------------------------------
// public functions

void console_defaults(struct console *cons)
{
    cons->state = S_NORM;
    cons->cols = g_vga->cols;
    cons->rows = g_vga->rows;
    for (int i = 0; i < MAX_TABSTOP; i++) {
        cons->tabstops[i] = (((i + 1) % TABSTOP_WIDTH) == 0);
    }
    memset(cons->csiparam, -1, sizeof(cons->csiparam));
    cons->paramidx = 0;
    cons->blink_on = false;
    cons->need_wrap = false;
    cons->attr.bg = VGA_BLACK;
    cons->attr.fg = VGA_WHITE;
    cons->attr.bright = false;
    cons->attr.faint = false;
    cons->attr.italic = false;
    cons->attr.underline = false;
    cons->attr.blink = false;
    cons->attr.invert = false;
    cons->cursor.x = 0;
    cons->cursor.y = 0;
    cons->cursor.shape = g_vga->orig_cursor_shape;
    cons->cursor.hidden = false;
    cons->csi_defaults.attr = cons->attr;
    cons->csi_defaults.cursor = cons->cursor;
    save_console(cons);
}

extern int tty_open_internal(struct tty *tty);

int switch_console(int num)
{
    if (num <= 0 || num > NR_CONSOLE) {
        return -EINVAL;
    }

    uint32_t flags;
    cli_save(flags);

    struct console *curr = current_console();
    struct console *next = get_console(num);
    struct tty *tty = NULL;

    if (get_tty(__mkdev(TTY_MAJOR, num), &tty)) {
        panic("tty%d not found", num);
    }
    if (tty_open_internal(tty)) {
        panic("could not switch consoles -- unable to open tty%d", num);
    }

    curr->framebuf = get_console_fb(curr->number);
    next->framebuf = get_console_fb(next->number);

    uint32_t pgdir_addr = 0;
    store_cr3(pgdir_addr);
    pgdir_addr = __phys_to_virt(pgdir_addr);

#if HIGHER_GROUND
    // enable kernel identity mapping so we can operate on page tables
    pde_t *ident_pde = (pde_t *) pgdir_addr;
    *ident_pde = __mkpde(KERNEL_PGTBL, _PAGE_RW);
#endif

    // identity map old frame buffer, so it will write to back buffer
    for (int i = 0; i < FB_SIZE_PAGES; i++) {
        uint32_t fb_page = (uint32_t) curr->framebuf + (i << PAGE_SHIFT);
        pte_t *pte = pte_offset((pde_t *) pgdir_addr, fb_page);
        *pte = __mkpte(__virt_to_phys(fb_page), _PAGE_RW);
    }
    flush_tlb();

    // swap buffers
    memcpy(curr->framebuf, (void *) g_vga->fb_info.framebuf, PAGE_SIZE * FB_SIZE_PAGES);
    memcpy((void *) g_vga->fb_info.framebuf, next->framebuf, PAGE_SIZE * FB_SIZE_PAGES);
    curr = next;

    // map new frame buffer to VGA
    for (int i = 0; i < FB_SIZE_PAGES; i++) {
        uint32_t fb_page = (uint32_t) curr->framebuf + (i << PAGE_SHIFT);
        uint32_t vga_page = g_vga->fb_info.framebuf + (i << PAGE_SHIFT);
        pte_t *pte = pte_offset((pde_t *) pgdir_addr, fb_page);
        *pte = __mkpte(__virt_to_phys(vga_page), _PAGE_RW);
    }

#if HIGHER_GROUND
    pde_clear(ident_pde);
#endif

    flush_tlb();

    // update VGA state
    g_vga->active_console = curr->number;
    vga_blink_enable(curr);
    vga_cursor_enable(curr);
    vga_set_cursor_shape(curr);
    vga_set_cursor_pos(curr);

    // a console is fully initialized once we've switch to it at least once :-)
    curr->initialized = true;

    restore_flags(flags);
    return 0;
}

struct console * current_console(void)
{
    return get_console(0);
}

struct console * get_console(int num)
{
    // 0 = current active console

    if (num < 0 || num > NR_CONSOLE) {
        panic("attempt to get invalid console!");
    }

    if (num == 0) {
        num = g_vga->active_console;
        assert(num > 0 && (num - 1) < NR_CONSOLE);
    }

    struct console *cons = &g_consoles[num - 1];
    if (cons->initialized) {
        assert(cons->number == num);
    }

    return cons;
}

void * get_console_fb(int num)
{
    char *fb = (char *) g_vga->fb_info.framebuf;
    fb += (num * FB_SIZE_PAGES) << PAGE_SHIFT;

    return fb;
}

void console_save(struct console *cons, struct console_save_state *save)
{
    memcpy(save->tabstops, cons->tabstops, MAX_TABSTOP);
    save->blink_on = cons->blink_on;
    save->attr = cons->attr._value;
    save->cursor = cons->cursor._value;
}

void console_restore(struct console *cons, struct console_save_state *save)
{
    memcpy(cons->tabstops, save->tabstops, MAX_TABSTOP);
    cons->blink_on = save->blink_on;
    cons->attr._value = save->attr;
    cons->cursor._value = save->cursor;

    if (is_current(cons)) {
        vga_blink_enable(cons);
        vga_cursor_enable(cons);
        vga_set_cursor_shape(cons);
        vga_set_cursor_pos(cons);
    }
}

int console_write(struct console *cons, const char *buf, size_t count)
{
    const char *p;

    if (!cons || !buf) {
        return -EINVAL;
    }

    p = buf;
    while (p < buf + count) {
        p += console_putchar(cons, *p);
    }

    return count;
}

int console_putchar(struct console *cons, char c)
{
    bool update_char = false;
    bool update_attr = false;
    bool update_cursor_pos = true;
    uint16_t char_pos;

    // prevent reentrancy
    if (test_and_set_bit(&cons->printing, 0)) {
        return 0;
    }

#if E9_HACK
    if (is_syscon(cons)) {
        outb(0xE9, c);
    }
#endif

    // handle escape sequences if not a control character
    if (!iscntrl(c)) {
        switch (cons->state) {
            case S_ESC:
                esc(cons, c);
                goto write_vga;
            case S_CSI:
                csi(cons, c);
                goto write_vga;
            case S_NORM:
                break;
            default:
                cons->state = S_NORM;
                break;
        }
    }

    // control characters
    switch (c) {
        case '\a':      // ^G - BEL - beep!
            beep(BELL_FREQ, BELL_TIME);         // TODO: ioctl to control beep tone/time
            break;
        case '\b':      // ^H - BS - backspace
            backspace(cons);
            break;
        case '\t':      // ^I - HT - horizontal tab
            tab(cons);
            break;
        case '\n':      // ^J - LF - line feed
            __fallthrough;
        case '\v':      // ^K - VT - vertical tab
        case '\f':      // ^L - FF - form feed
            line_feed(cons);
            break;
        case '\r':      // ^M - CR -  carriage return
            carriage_return(cons);
            break;

        case ASCII_CAN: // ^X - CAN - cancel escape sequence
            cons->state = S_NORM;
            goto done;
        case '\e':      // ^[ - ESC - start escape sequence
            cons->state = S_ESC;
            goto done;

        default:        // everything else
            // ignore unhandled control characters
            if (iscntrl(c)) {
                goto done;
            }

            update_char = true;
            update_attr = true;

            // handle deferred wrap
            if (cons->need_wrap) {
                carriage_return(cons);
                line_feed(cons);
            }

            // determine character position
            char_pos = xy2pos(cons->cols, cons->cursor.x, cons->cursor.y);

            // advance cursor
            cons->cursor.x++;
            if (cons->cursor.x >= cons->cols) {
                // if the cursor is at the end of the line, prevent
                // the display from scrolling one line (wrapping) until
                // the next character is received so we aren't left with
                // an unnecessary blank line
                cons->cursor.x--;
                cons->need_wrap = true;
                update_cursor_pos = false;
            }
            break;
    }

write_vga:
    if (update_char) {
        set_fb_char(cons, char_pos, c);
    }
    if (update_attr) {
        if (cons->attr.bright && cons->attr.faint) {
            cons->attr.bright = false;      // faint overrides bright
        }
        set_fb_attr(cons, char_pos, cons->attr);
    }
    if (update_cursor_pos && is_current(cons)) {
        vga_set_cursor_pos(cons);
    }

done:
    clear_bit(&cons->printing, 0);
    return 1;
}

// ----------------------------------------------------------------------------
// private functions

static void esc(struct console *cons, char c)
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
            line_feed(cons);
            break;
        case 'E':       // ESC E - NEL - newline (CRLF)
            carriage_return(cons);
            line_feed(cons);
            break;
        case 'H':       // ESC H - HTS - set tab stop
            cons->tabstops[cons->cursor.x] = 1;
            break;
        case 'M':       // ESC M - RI - reverse line feed
            reverse_linefeed(cons);
            break;
        case '[':       // ESC [ - CSI - control sequence introducer
            memset(cons->csiparam, 0xFF, sizeof(cons->csiparam));
            cons->paramidx = 0;
            cons->state = S_CSI;
            return;

        //
        // "Custom" console-related sequences
        //
        case '3':       // ESC 3    disable blink
            cons->blink_on = false;
            if (is_current(cons)) {
                vga_blink_enable(cons);
            }
            break;
        case '4':       // ESC 4    enable blink
            cons->blink_on = true;
            if (is_current(cons)) {
                vga_blink_enable(cons);
            }
            break;
        case '5':       // ESC 5    hide cursor
            cons->cursor.hidden = true;
            if (is_current(cons)) {
                vga_cursor_enable(cons);
            }
            break;
        case '6':       // ESC 6    show cursor
            cons->cursor.hidden = false;
            if (is_current(cons)) {
                vga_cursor_enable(cons);
            }
            break;
        case '7':       // ESC 7    save console
            save_console(cons);
            break;
        case '8':       // ESC 8    restore console
            restore_console(cons);
            break;
        case 'c':       // ESC c    reset console
            reset(cons);
            break;
        case 'h':       // ESC h    clear tab stop
            cons->tabstops[cons->cursor.x] = 0;     // TODO: replace with ESC [0g (clear current) and ESC [3g (clear all)
            break;
        default:
            break;
    }

    cons->need_wrap = false;
    cons->state = S_NORM;
}

static void csi(struct console *cons, char c)
{
    //
    // ANSI Control Sequences
    //
    // https://www.man7.org/linux/man-pages/man3/termios.3.html
    // https://en.wikipedia.org/wiki/ANSI_escape_code
    //

    #define param_minimum(index,value)          \
    do {                                        \
        if (cons->csiparam[index] < (value)) {  \
            cons->csiparam[index] = (value);    \
        }                                       \
    } while (0)

    #define param_maximum(index,value)          \
    do {                                        \
        if (cons->csiparam[index] > (value)) {  \
            cons->csiparam[index] = (value);    \
        }                                       \
    } while (0)

    switch (c)
    {
        //
        // "Standard" sequences
        //
        case 'A':       // CSI n A  - CUU - move cursor up n rows
            param_minimum(0, 1);
            cursor_up(cons, cons->csiparam[0]);
            goto csi_done;
        case 'B':       // CSI n B  - CUD - move cursor down n rows
            param_minimum(0, 1);
            cursor_down(cons, cons->csiparam[0]);
            goto csi_done;
        case 'C':       // CSI n C  - CUF - move cursor right (forward) n columns
            param_minimum(0, 1);
            cursor_right(cons, cons->csiparam[0]);
            goto csi_done;
        case 'D':       // CSI n D  - CUB - move cursor left (back) n columns
            param_minimum(0, 1);
            cursor_left(cons, cons->csiparam[0]);
            goto csi_done;
        case 'E':       // CSI n E  - CNL - move cursor to beginning of line, n rows down
            param_minimum(0, 1);
            cons->cursor.x = 0;
            cursor_down(cons, cons->csiparam[0]);
            goto csi_done;
        case 'F':       // CSI n F  - CPL - move cursor to beginning of line, n rows up
            param_minimum(0, 1);
            cons->cursor.x = 0;
            cursor_up(cons, cons->csiparam[0]);
            goto csi_done;
        case 'G':       // CSI n G  - CHA - move cursor to column n
            param_minimum(0, 1);
            param_maximum(0, cons->cols);
            cons->cursor.x = cons->csiparam[0] - 1;
            goto csi_done;
        case 'H':       // CSI n ; m H - CUP - move cursor to row n, column m
            param_minimum(0, 1);
            param_minimum(1, 1);
            param_maximum(0, cons->rows);
            param_maximum(1, cons->cols);
            cons->cursor.y = cons->csiparam[0] - 1;
            cons->cursor.x = cons->csiparam[1] - 1;
            goto csi_done;
        case 'J':       // CSI n J  - ED - erase in display (n = mode)
            param_minimum(0, 0);
            erase(cons, cons->csiparam[0]);
            goto csi_done;
        case 'K':       // CSI n K  - EL- erase in line (n = mode)
            param_minimum(0, 0);
            erase_line(cons, cons->csiparam[0]);
            goto csi_done;
        case 'S':       // CSI n S  - SU - scroll n lines
            param_minimum(0, 1);
            scroll(cons, cons->csiparam[0]);
            goto csi_done;
        case 'T':       // CSI n T  - ST - reverse scroll n lines
            param_minimum(0, 1);
            scroll(cons, -cons->csiparam[0]);     // note the negative for reverse!
            goto csi_done;
        case 'm':       // CSI n m  - SGR - set graphics attribute
            for (int i = 0; i <= cons->paramidx; i++){
                param_minimum(i, 0);
                csi_m(cons, cons->csiparam[i]);
            }
            goto csi_done;

        //
        // Custom (or "private") sequences
        //
        case 's':       // CSI s        save cursor position
            cursor_save(cons);
            goto csi_done;
        case 'u':       // CSI u        restore cursor position
            cursor_restore(cons);
            goto csi_done;

        //
        // CSI params
        //
        case ';':   // parameter separator
            cons->paramidx++;
            if (cons->paramidx >= MAX_CSIPARAM) {
                goto csi_done;  // too many params! cancel
            }
            goto csi_next;
        default:    // parameter
            if (isdigit(c)) {
                if (cons->csiparam[cons->paramidx] == -1) {
                    cons->csiparam[cons->paramidx] = 0;
                }
                cons->csiparam[cons->paramidx] *= 10;
                cons->csiparam[cons->paramidx] += (c - '0');
                goto csi_next;
            }
            goto csi_done;  // invalid param char
    }

csi_done:   // CSI processing done
    cons->need_wrap = false;
    cons->state = S_NORM;

csi_next:   // we need more CSI characters; do not alter console state
    return;

    #undef param_minimum
    #undef param_maximum
}

static void csi_m(struct console *cons, char p)
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
            cons->attr = cons->csi_defaults.attr;
            break;
        case 1:     // set bright (bold)
            cons->attr.bright = true;
            break;
        case 2:     // set faint (simulated with color)
            cons->attr.faint = true;
            break;
        case 3:     // set italic (simulated with color)
            cons->attr.italic = true;
            break;
        case 4:     // set underline (simulated with color)
            cons->attr.underline = true;
            break;
        case 5:     // set blink
            cons->attr.blink = true;
            break;
        case 7:     // set fg/bg color inversion
            cons->attr.invert = true;
            break;
        case 22:    // normal intensity (neither bright nor faint)
            cons->attr.bright = false;
            cons->attr.faint = false;
            break;
        case 23:    // disable italic
            cons->attr.italic = false;
            break;
        case 24:    // disable underline
            cons->attr.underline = false;
            break;
        case 25:    // disable blink
            cons->attr.blink = false;
            break;
        case 27:    // disable fg/bg inversion
            cons->attr.invert = false;
            break;
        default:
            // colors
            if (p >= 30 && p <= 37) cons->attr.fg = CSI_COLORS[p - 30];
            if (p >= 40 && p <= 47) cons->attr.bg = CSI_COLORS[p - 40];
            if (p == 39) cons->attr.fg = cons->csi_defaults.attr.fg;
            if (p == 49) cons->attr.bg = cons->csi_defaults.attr.bg;
            if (p >= 90 && p <= 97) {
                cons->attr.fg = CSI_COLORS[p - 90];
                cons->attr.bright = 1;
            }
            if (p >= 100 && p <= 107) {
                cons->attr.bg = CSI_COLORS[p - 100];
                cons->attr.bright = !cons->attr.blink;  // blink overrides bright
            }
            break;
    }
}

static void reset(struct console *cons)
{
    console_defaults(cons);
    erase(cons, 2);
    if (is_current(cons)) {
        vga_blink_enable(cons);
        vga_cursor_enable(cons);
        vga_set_cursor_shape(cons);
        vga_set_cursor_pos(cons);
    }
}

static void save_console(struct console *cons)
{
    console_save(cons, &cons->saved_state);
}

static void restore_console(struct console *cons)
{
    console_restore(cons, &cons->saved_state);
}

static void cursor_save(struct console *cons)
{
    cons->saved_state.cursor = cons->cursor._value;
}

static void cursor_restore(struct console *cons)
{
    cons->cursor._value = cons->saved_state.cursor;
    if (is_current(cons)) {
        vga_cursor_enable(cons);
        vga_set_cursor_shape(cons);
        vga_set_cursor_pos(cons);
    }
}

static void backspace(struct console *cons)
{
    cursor_left(cons, 1);
    cons->need_wrap = false;
}

static void carriage_return(struct console *cons)
{
    cons->cursor.x = 0;
    cons->need_wrap = false;
}

static void line_feed(struct console *cons)
{
    if (++cons->cursor.y >= cons->rows) {
        scroll(cons, 1);
        cons->cursor.y--;
    }
    cons->need_wrap = false;
}

static void reverse_linefeed(struct console *cons)
{
    if (--cons->cursor.y < 0) {     // TODO: fix so it doesn't wrap
        scroll(cons, -1);
        cons->cursor.y++;
    }
    cons->need_wrap = false;
}

static void tab(struct console *cons)
{
    while (cons->cursor.x < cons->cols) {
        if (cons->tabstops[++cons->cursor.x]) {
            break;
        }
    }

    if (cons->cursor.x >= cons->cols) {
        cons->cursor.x = cons->cols - 1;
    }
}

static void scroll(struct console *cons, int n)   // n < 0 is reverse scroll
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
    if (n > cons->rows) {
        n = cons->rows;
    }
    if (n == 0) {
        return;
    }

    n_blank = n * cons->cols;
    n_cells = (cons->rows * cons->cols) - n_blank;
    n_bytes = n_cells * sizeof(struct vga_cell);

    src_end = &((struct vga_cell *) cons->framebuf)[n_blank];
    src = (reverse) ? cons->framebuf : src_end;
    dst = (reverse) ? src_end : cons->framebuf;
    memmove(dst, src, n_bytes);

    for (i = 0; i < n_blank; i++) {
        int pos = (reverse) ? i : n_cells + i;
        set_fb_char(cons, pos, ' ');
        set_fb_attr(cons, pos, cons->attr);
    }
}

static void erase(struct console *cons, int mode)
{
    int start;
    int count;
    int pos = xy2pos(cons->cols, cons->cursor.x, cons->cursor.y);
    int area = cons->rows * cons->cols;

    switch (mode) {
        case 0:     // erase screen from cursor down
            start = pos;
            count = area - pos;
            break;
        case 1:     // erase screen from cursor up
            start = 0;
            count = pos + 1;
            break;
        case 2:     // erase entire screen
        default:
            start = 0;
            count = area;
            break;
    }

    for (int i = 0; i < count; i++) {
        set_fb_char(cons, start + i, ' ');
        set_fb_attr(cons, start + i, cons->attr);
    }
}

static void erase_line(struct console *cons, int mode)
{
    int start;
    int count;
    int pos = xy2pos(cons->cols, cons->cursor.x, cons->cursor.y);

    switch (mode) {
        case 0:    // erase line from cursor down
            start = pos;
            count = cons->cols - (pos % cons->cols);
            break;
        case 1:      // erase line from cursor up
            start = xy2pos(cons->cols, 0, cons->cursor.y);
            count = (pos % cons->cols) + 1;
            break;
        case 2:     // erase entire line
        default:
            start = xy2pos(cons->cols, 0, cons->cursor.y);
            count = cons->cols;
    }

    for (int i = 0; i < count; i++) {
        set_fb_char(cons, start + i, ' ');
        set_fb_attr(cons, start + i, cons->attr);
    }
}

static void cursor_up(struct console *cons, int n)
{
    if (cons->cursor.y - n > 0) {
        cons->cursor.y -= n;
    }
    else {
        cons->cursor.y = 0;
    }
}

static void cursor_down(struct console *cons, int n)
{
    if (cons->cursor.y + n < cons->rows - 1) {
        cons->cursor.y += n;
    }
    else {
        cons->cursor.y = cons->rows - 1;
    }
}

static void cursor_left(struct console *cons, int n)
{
    if (cons->cursor.x - n > 0) {
        cons->cursor.x -= n;
    }
    else {
        cons->cursor.x = 0;
    }
}

static void cursor_right(struct console *cons, int n)
{
    if (cons->cursor.x + n < cons->cols - 1) {
        cons->cursor.x += n;
    }
    else {
        cons->cursor.x = cons->cols - 1;
    }
}

static uint16_t xy2pos(uint16_t ncols, uint16_t x, uint16_t y)
{
    return y * ncols + x;
}

static void set_fb_char(struct console *cons, uint16_t pos, char c)
{
    ((struct vga_cell *) cons->framebuf)[pos].ch = c;
}

static void set_fb_attr(struct console *cons, uint16_t pos, struct _char_attr attr)
{
    struct vga_attr *vga_attr;
    vga_attr = &((struct vga_cell *) cons->framebuf)[pos].attr;

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

// ----------------------------------------------------------------------------

static void vga_blink_enable(const struct console *cons)
{
    uint32_t flags;
    uint8_t modectl;

    cli_save(flags);
    modectl = vga_attr_read(VGA_ATTR_REG_MODE);

    if (cons->blink_on) {
        modectl |= VGA_ATTR_FLD_MODE_BLINK;
    }
    else {
        modectl &= ~VGA_ATTR_FLD_MODE_BLINK;
    }

    vga_attr_write(VGA_ATTR_REG_MODE, modectl);
    restore_flags(flags);
}

static void vga_cursor_enable(const struct console *cons)
{
    uint32_t flags;
    uint8_t css;
    cli_save(flags);
    css = vga_crtc_read(VGA_CRTC_REG_CSS);

    if (cons->cursor.hidden) {
        css |= VGA_CRTC_FLD_CSS_CD_MASK;
    }
    else {
        css &= ~VGA_CRTC_FLD_CSS_CD_MASK;
    }

    vga_crtc_write(VGA_CRTC_REG_CSS, css);
    restore_flags(flags);
}

static void vga_set_cursor_pos(const struct console *cons)
{
    uint32_t flags;
    uint16_t pos;

    pos = xy2pos(cons->cols, cons->cursor.x, cons->cursor.y);

    cli_save(flags);
    vga_crtc_write(VGA_CRTC_REG_CL_HI, pos >> 8);
    vga_crtc_write(VGA_CRTC_REG_CL_LO, pos & 0xFF);
    restore_flags(flags);
}

static void vga_set_cursor_shape(const struct console *cons)
{
    uint32_t flags;
    uint8_t start, end;
    uint8_t css, cse;

    start = ((uint16_t) cons->cursor.shape) & 0xFF;
    end = ((uint16_t) cons->cursor.shape) >> 8;

    cli_save(flags);
    css = vga_crtc_read(VGA_CRTC_REG_CSS) & ~VGA_CRTC_FLD_CSS_CSS_MASK;
    cse = vga_crtc_read(VGA_CRTC_REG_CSE) & ~VGA_CRTC_FLD_CSE_CSE_MASK;
    css |= (start & VGA_CRTC_FLD_CSS_CSS_MASK);
    cse |= (end   & VGA_CRTC_FLD_CSE_CSE_MASK);
    vga_crtc_write(VGA_CRTC_REG_CSS, css);
    vga_crtc_write(VGA_CRTC_REG_CSE, cse);
    restore_flags(flags);
}


