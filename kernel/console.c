/* =============================================================================
 * Copyright (C) 2020-2024 Wes Hampson. All Rights Reserved.
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
 *         File: kernel/console.c
 *      Created: March 26, 2023
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <ohwes.h>
#include <boot.h>
#include <console.h>
#include <ctype.h>
#include <errno.h>
#include <kernel.h>
#include <vga.h>
#include <io.h>

#define NUM_CONSOLES        8

#define BLANK_CHAR          ' '
#define CSIPARAM_EMPTY      (-1)
#define CSIPARAM_SEPARATOR  ';'

#define ERASE_DOWN          0
#define ERASE_UP            1
#define ERASE_ALL           2

// TODO: set via ioctl
#define BELL_FREQ           750     // Hz
#define BELL_TIME           50      // ms
#define ALERT_FREQ          1000
#define ALERT_TIME          50
#define CURSOR_ULINE        0x0E0C  // scan line start = 12, end = 14
#define CURSOR_BLOCK        0x0F00  // scan line start = 0,  end = 15

struct vga {
    int active_console;
    int rows, cols;
    void *framebuf;
};

// global vars
struct vga g_vga = { };
struct console g_consoles[NUM_CONSOLES] = { };

struct console * current_console(void)
{
    return &g_consoles[g_vga.active_console];
}

struct console * get_console(int num)
{
    if (num < 0 || num >= NUM_CONSOLES) {
        return NULL;
    }
    return &g_consoles[num];
}

//
// This is totally something I yoinked from Linux. It allows for n consoles and
// uses clever yet slightly evil macros which allow member access to the active
// console in the console array. I don't know if I'll keep it but for now it'll
// do.
//
// TODO: should probably get rid of this construct...
#define m_initialized       (current_console()->initialized)
#define m_state             (current_console()->state)
#define m_cols              (current_console()->cols)
#define m_rows              (current_console()->rows)
#define m_framebuf          (current_console()->framebuf)
#define m_inputq            (&current_console()->inputq)
#define m_tabstops          (current_console()->tabstops)
#define m_csiparam          (current_console()->csiparam)
#define m_paramidx          (current_console()->paramidx)
#define m_currparam         (m_csiparam[m_paramidx])
#define m_need_wrap         (current_console()->need_wrap)
#define m_blink_on          (current_console()->blink_on)
#define m_termios           (current_console()->termios)
#define m_attr              (current_console()->attr)
#define m_cursor            (current_console()->cursor)
#define m_saved_state       (current_console()->saved_state)
#define m_csi_defaults      (current_console()->csi_defaults)

#define has_oflag(oflag)    (has_flag(m_termios.c_oflag, oflag))

enum console_state {
    S_NORM,
    S_ESC,
    S_CSI
};

// console feature escape sequences
static void reset(void);                    // ESC c
static void vga_disable_char_blink(void);   // ESC 3
static void vga_enable_char_blink(void);    // ESC 4
static void vga_hide_cursor(void);          // ESC 5
static void vga_show_cursor(void);          // ESC 6
static void save_console(void);             // ESC 7
static void restore_console(void);          // ESC 8
static void cursor_save(void);              // ESC [s
static void cursor_restore(void);           // ESC [u
static void set_cursor_state(bool update_shape);

// character handling
static void write_char(char c);
static void esc(char c);                    // ^[ (ESC)
static void csi(char c);                    // ESC [
static void csi_m(char p);                  // ESC [<params>m
static void backspace(void);                // ^H
static void tab(void);                      // ^I
static void line_feed(void);                // ^J
static void reverse_linefeed(void);         // ESC M
static void carriage_return(void);          // ^M
static void scroll(int n);                  // ESC [<n>S / ESC [<n>T
static void erase(int mode);                // ESC [<n>J
static void erase_line(int mode);           // ESC [<n>K
static void cursor_up(int n);               // ESC [<n>A
static void cursor_down(int n);             // ESC [<n>B
static void cursor_right(int n);            // ESC [<n>C
static void cursor_left(int n);             // ESC [<n>D

// screen positioning
static void pos2xy(uint16_t pos, uint16_t *x, uint16_t *y);
static uint16_t xy2pos(uint16_t x, uint16_t y);

// VGA progrmming
static void vga_set_char(uint16_t pos, char c);
static void vga_set_attr(uint16_t pos, struct _char_attr attr);
static uint16_t vga_get_cursor(void);
static uint16_t vga_get_cursor_shape(void);
static void vga_set_cursor(uint16_t pos);
static void vga_set_cursor_shape(uint8_t start, uint8_t end);

// ----------------------------------------------------------------------------
// initialization

extern void init_kb(const struct boot_info *info);

void init_console(const struct boot_info *info)
{
    zeromem(&g_vga, sizeof(struct vga));
    zeromem(g_consoles, sizeof(struct console) * NUM_CONSOLES);

    // get VGA info from boot info
    g_vga.active_console = 0;
    g_vga.rows = info->vga_rows;
    g_vga.cols = info->vga_cols;

    // determine frame buffer address
    uint8_t gfx_misc = vga_grfx_read(VGA_GRFX_REG_MISC);
    uint8_t ram_select = (gfx_misc & 0x0C) >> 2;
    switch (ram_select) {
        case 0: g_vga.framebuf = (void *) 0xA0000; break; // A0000-BFFFF (128K)
        case 1: g_vga.framebuf = (void *) 0xA0000; break; // A0000-AFFFF (64K)
        case 2: g_vga.framebuf = (void *) 0xB0000; break; // B0000-B7FFF (32K)
        case 3: g_vga.framebuf = (void *) 0xB8000; break; // B8000-BFFFF (32K)
    }

    // write all console defaults
    for (int i = 0; i < NUM_CONSOLES; i++) {
        console_defaults(&g_consoles[i]);
    }

    // read cursor attributes leftover from BIOS
    m_cursor.shape = vga_get_cursor_shape();
    pos2xy(vga_get_cursor(), &m_cursor.x, &m_cursor.y);

    // create a restore point
    save_console();

    // safe to print now
    kprint("\r\n\e4\e6");
    kprint("\e[0;1m" OS_NAME " " OS_VERSION ", build: " OS_BUILDDATE " ]]\e[0m\n");

    // get the keyboard working
    init_kb(info);

    // done!
    m_initialized = true;
}

// ----------------------------------------------------------------------------
// public, console-independent functions

void console_defaults(struct console *cons)
{
    cons->state = S_NORM;
    cons->cols = g_vga.cols;
    cons->rows = g_vga.rows;
    cons->framebuf = PAGE_OFFSET + g_vga.framebuf;
    char_queue_init(&cons->inputq, cons->_ibuf, INPUT_BUFFER_SIZE);
    for (int i = 0; i < MAX_TABSTOPS; i++) {
        m_tabstops[i] = (((i + 1) % TABSTOP_WIDTH) == 0);
    }
    memset(cons->csiparam, CSIPARAM_EMPTY, MAX_CSIPARAMS);
    cons->paramidx = 0;
    cons->blink_on = false;
    cons->need_wrap = false;
    cons->termios.c_oflag = DEFAULT_OFLAG;
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
    cons->cursor.shape = CURSOR_ULINE;
    cons->cursor.hidden = false;
    cons->csi_defaults.attr = cons->attr;
    cons->csi_defaults.cursor = cons->cursor;
    save_console();
}

int console_read(struct console *cons, char *buf, size_t count)
{
    int nread;
    uint32_t flags;

    if (!buf) {
        return -EINVAL;
    }

    // TODO: make sure this comes from the correct console for the
    // calling process!!

    nread = 0;
    while (count--) {
        spin(char_queue_empty(m_inputq));    // block until a character appears
        // TODO: allow nonblocking input

        cli_save(flags);
        *buf++ = char_queue_get(m_inputq);
        restore_flags(flags);
        nread++;
    }


    return nread;
}

int console_write(struct console *cons, const char *buf, size_t count)
{
    if (!cons || !buf) {
        return -EINVAL;
    }

    // TODO: make sure this goes to the correct console for the
    // calling process!!

    for (int i = 0; i < count; i++) {
        write_char(buf[i]);
#if E9_HACK
        if (cons == get_console(0)) {
            outb(0xE9, buf[i]);
        }
#endif
    }

    return count;
}

int console_recv(struct console *cons, char c)  // called from ps2kb.c
{
    if (!cons) {
        return -EINVAL;
    }

    if (char_queue_full(&cons->inputq)) {
        kprint("console: input buffer full!\n");
        beep(ALERT_FREQ, ALERT_TIME);
        return 0;
    }

    // TODO: input processing, leave space for \n, etc.
    char_queue_put(&cons->inputq, (char) c);
    return 1;
}

// ----------------------------------------------------------------------------
// static functions that operate on the active console only

static void reset(void)
{
    console_defaults(current_console());
    set_cursor_state(true);
    erase(ERASE_ALL);
}

static void save_console(void)
{
    memcpy(&m_saved_state.tabstops, &m_tabstops, MAX_TABSTOPS);
    m_saved_state.blink_on = m_blink_on;
    m_saved_state.attr = m_attr;
    cursor_save();
}

static void restore_console(void)
{
    memcpy(&m_tabstops, &m_saved_state.tabstops, MAX_TABSTOPS);
    m_blink_on = m_saved_state.blink_on;
    m_attr = m_saved_state.attr;
    cursor_restore();
}

static void cursor_save(void)
{
    m_saved_state.cursor = m_cursor;
}

static void cursor_restore(void)
{
    m_cursor = m_saved_state.cursor;
}

static void set_cursor_state(bool update_shape)
{
    vga_set_cursor(xy2pos(m_cursor.x, m_cursor.y));
    if (update_shape) {
        vga_set_cursor_shape(m_cursor.shape & 0xFF, m_cursor.shape >> 8);
    }
}

static void write_char(char c)
{
    bool update_char = false;
    bool update_attr = false;
    bool update_cursor_pos = true;
    bool update_cursor_shape = false;
    uint16_t char_pos;

    // handle escape sequences if not a control character
    if (!iscntrl(c)) {
        switch (m_state) {
            case S_ESC:
                esc(c);
                goto write_vga;
            case S_CSI:
                csi(c);
                goto write_vga;
            case S_NORM:
                break;
            default:
                m_state = S_NORM;
                panic("invalid console state!");
        }
    }

    // control characters
    switch (c) {
        case '\a':      // ^G - BEL - beep!
            beep(BELL_FREQ, BELL_TIME);
            break;
        case '\b':      // ^H - BS - backspace
            backspace();
            break;
        case '\t':      // ^I - HT - horizontal tab
            tab();
            break;
        case '\n':      // ^J - LF - line feed
            if (has_oflag(OPOST) && has_oflag(ONLCR)) {
                carriage_return();
            }
            __fallthrough;
        case '\v':      // ^K - VT - vertical tab
        case '\f':      // ^L - FF - form feed
            line_feed();
            break;
        case '\r':      // ^M - CR -  carriage return
            if (has_oflag(OPOST) && has_oflag(OCRNL)) {
                line_feed();
            }
            else {
                carriage_return();
            }
            break;

        case ASCII_CAN: // ^X - CAN - cancel escape sequence
            m_state = S_NORM;
            goto done;
        case '\e':      // ^[ - ESC - start escape sequence
            m_state = S_ESC;
            goto done;

        default:        // everything else
            if (iscntrl(c)) {
                goto done;
            }

            update_char = true;
            update_attr = true;

            // handle deferred wrap
            if (m_need_wrap) {
                carriage_return();
                line_feed();
            }

            // determine character position
            char_pos = xy2pos(m_cursor.x, m_cursor.y);

            // advance cursor
            m_cursor.x++;
            if (m_cursor.x >= m_cols) {
                // if the cursor is at the end of the line, prevent
                // the display from scrolling one line (wrapping) until
                // the next character is received so we aren't left with
                // an unnecessary blank line
                m_cursor.x--;
                m_need_wrap = true;
                update_cursor_pos = false;
            }
            break;
    }

write_vga:
    if (update_char) {
        vga_set_char(char_pos, c);
    }
    if (update_attr) {
        if (m_attr.bright && m_attr.faint) {
            m_attr.bright = false;      // faint overrides bright
        }
        vga_set_attr(char_pos, m_attr);
    }
    if (update_cursor_pos) {
        bool update_shape = update_cursor_shape && !m_cursor.hidden;
        set_cursor_state(update_shape);
    }

done:
    return;
}

static void esc(char c)
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
            line_feed();
            break;
        case 'E':       // ESC E - NEL - newline (CRLF)
            carriage_return();
            line_feed();
            break;
        case 'H':       // ESC H - HTS - set tab stop
            m_tabstops[m_cursor.x] = 1;
            break;
        case 'M':       // ESC M - RI - reverse line feed
            reverse_linefeed();
            break;
        case '[':       // ESC [ - CSI - control sequence introducer
            memset(m_csiparam, CSIPARAM_EMPTY, MAX_CSIPARAMS);
            m_paramidx = 0;
            m_state = S_CSI;
            return;

        //
        // "Custom" console-related sequences
        //
        case '3':       // ESC 3    disable blink
            m_blink_on = false;
            vga_disable_char_blink();
            break;
        case '4':       // ESC 4    enable blink
            m_blink_on = true;
            vga_enable_char_blink();
            break;
        case '5':       // ESC 5    hide cursor
            m_cursor.hidden = true;
            vga_hide_cursor();
            break;
        case '6':       // ESC 6    show cursor
            m_cursor.hidden = false;
            vga_show_cursor();
            break;
        case '7':       // ESC 7    save console
            save_console();
            break;
        case '8':       // ESC 8    restore console
            restore_console();
            break;
        case 'c':       // ESC c    reset console
            reset();
            break;
        case 'h':       // ESC h    clear tab stop
            m_tabstops[m_cursor.x] = 0;
            break;
        default:
            break;
    }

    m_need_wrap = false;
    m_state = S_NORM;
}

static void csi(char c)
{
    //
    // ANSI Control Sequences
    //
    // https://www.man7.org/linux/man-pages/man3/termios.3.html
    // https://en.wikipedia.org/wiki/ANSI_escape_code
    //

    #define param_minimum(index,value)      \
    do {                                    \
        if (m_csiparam[index] < (value)) {  \
            m_csiparam[index] = (value);    \
        }                                   \
    } while (0)

    #define param_maximum(index,value)      \
    do {                                    \
        if (m_csiparam[index] > (value)) {  \
            m_csiparam[index] = (value);    \
        }                                   \
    } while (0)

    switch (c)
    {
        //
        // "Standard" sequences
        //
        case 'A':       // CSI n A  - CUU - move cursor up n rows
            param_minimum(0, 1);
            cursor_up(m_csiparam[0]);
            goto csi_done;
        case 'B':       // CSI n B  - CUD - move cursor down n rows
            param_minimum(0, 1);
            cursor_down(m_csiparam[0]);
            goto csi_done;
        case 'C':       // CSI n C  - CUF - move cursor right (forward) n columns
            param_minimum(0, 1);
            cursor_right(m_csiparam[0]);
            goto csi_done;
        case 'D':       // CSI n D  - CUB - move cursor left (back) n columns
            param_minimum(0, 1);
            cursor_left(m_csiparam[0]);
            goto csi_done;
        case 'E':       // CSI n E  - CNL - move cursor to beginning of line, n rows down
            param_minimum(0, 1);
            m_cursor.x = 0;
            cursor_down(m_csiparam[0]);
            goto csi_done;
        case 'F':       // CSI n F  - CPL - move cursor to beginning of line, n rows up
            param_minimum(0, 1);
            m_cursor.x = 0;
            cursor_up(m_csiparam[0]);
            goto csi_done;
        case 'G':       // CSI n G  - CHA - move cursor to column n
            param_minimum(0, 1);
            param_maximum(0, m_cols);
            m_cursor.x = m_csiparam[0] - 1;
            goto csi_done;
        case 'H':       // CSI n ; m H - CUP - move cursor to row n, column m
            param_minimum(0, 1);
            param_minimum(1, 1);
            param_maximum(0, m_rows);
            param_maximum(1, m_cols);
            m_cursor.y = m_csiparam[0] - 1;
            m_cursor.x = m_csiparam[1] - 1;
            goto csi_done;
        case 'J':       // CSI n J  - ED - erase in display (n = mode)
            param_minimum(0, 0);
            erase(m_csiparam[0]);
            goto csi_done;
        case 'K':       // CSI n K  - EL- erase in line (n = mode)
            param_minimum(0, 0);
            erase_line(m_csiparam[0]);
            goto csi_done;
        case 'S':       // CSI n S  - SU - scroll n lines
            param_minimum(0, 1);
            scroll(m_csiparam[0]);
            goto csi_done;
        case 'T':       // CSI n T  - ST - reverse scroll n lines
            param_minimum(0, 1);
            scroll(-m_csiparam[0]);     // note the negative for reverse!
            goto csi_done;
        case 'm':       // CSI n m  - SGR - set graphics attribute
            for (int i = 0; i <= m_paramidx; i++){
                param_minimum(i, 0);
                csi_m(m_csiparam[i]);
            }
            goto csi_done;

        //
        // Custom (or "private") sequences
        //
        case 's':       // CSI s        save cursor position
            cursor_save();
            goto csi_done;
        case 'u':       // CSI u        restore cursor position
            cursor_restore();
            goto csi_done;

        //
        // CSI params
        //
        case CSIPARAM_SEPARATOR: // parameter separator
            m_paramidx++;
            if (m_paramidx >= MAX_CSIPARAMS) {
                // too many params! cancel
                goto csi_done;
            }
            goto csi_next;
        default:                // parameter
            if (isdigit(c)) {
                if (m_currparam == CSIPARAM_EMPTY) {
                    m_currparam = 0;
                }
                m_currparam *= 10;
                m_currparam += (c - '0');
                goto csi_next;
            }
            goto csi_done;  // invalid param char
    }

csi_done:   // CSI processing done
    m_need_wrap = false;
    m_state = S_NORM;

csi_next:   // we need more CSI characters; do not alter console state
    return;

    #undef param_minimum
    #undef param_maximum
}

static void csi_m(char p)
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
            m_attr = m_csi_defaults.attr;
            break;
        case 1:     // set bright (bold)
            m_attr.bright = true;
            break;
        case 2:     // set faint (simulated with color)
            m_attr.faint = true;
            break;
        case 3:     // set italic (simulated with color)
            m_attr.italic = true;
            break;
        case 4:     // set underline (simulated with color)
            m_attr.underline = true;
            break;
        case 5:     // set blink
            m_attr.blink = true;
            break;
        case 7:     // set fg/bg color inversion
            m_attr.invert = true;
            break;
        case 22:    // normal intensity (neither bright nor faint)
            m_attr.bright = false;
            m_attr.faint = false;
            break;
        case 23:    // disable italic
            m_attr.italic = false;
            break;
        case 24:    // disable underline
            m_attr.underline = false;
            break;
        case 25:    // disable blink
            m_attr.blink = false;
            break;
        case 27:    // disable fg/bg inversion
            m_attr.invert = false;
            break;
        default:
            // colors
            if (p >= 30 && p <= 37) m_attr.fg = CSI_COLORS[p - 30];
            if (p >= 40 && p <= 47) m_attr.bg = CSI_COLORS[p - 40];
            if (p == 39) m_attr.fg = m_csi_defaults.attr.fg;
            if (p == 49) m_attr.bg = m_csi_defaults.attr.bg;
            if (p >= 90 && p <= 97) {
                m_attr.fg = CSI_COLORS[p - 90];
                m_attr.bright = 1;
            }
            if (p >= 100 && p <= 107) {
                m_attr.bg = CSI_COLORS[p - 100];
                m_attr.bright = !m_attr.blink;  // blink overrides bright
            }
            break;
    }
}

static void backspace(void)
{
    cursor_left(1);
    m_need_wrap = false;
}

static void carriage_return(void)
{
    m_cursor.x = 0;
    m_need_wrap = false;
}

static void line_feed(void)
{
    if (++m_cursor.y >= m_rows) {
        scroll(1);
        m_cursor.y--;
    }
    m_need_wrap = false;
}

static void reverse_linefeed(void)
{
    if (--m_cursor.y < 0) {     // TODO: fix so it doesn't wrap
        scroll(-1);
        m_cursor.y++;
    }
    m_need_wrap = false;
}

static void tab(void)
{
    while (m_cursor.x < m_cols) {
        if (m_tabstops[++m_cursor.x]) {
            break;
        }
    }

    if (m_cursor.x >= m_cols) {
        m_cursor.x = m_cols - 1;
    }
}

static void scroll(int n)   // n < 0 is reverse scroll
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
    if (n > m_rows) {
        n = m_rows;
    }
    if (n == 0) {
        return;
    }

    n_blank = n * m_cols;
    n_cells = (m_rows * m_cols) - n_blank;
    n_bytes = n_cells * sizeof(struct vga_cell);

    src_end = &((struct vga_cell *) m_framebuf)[n_blank];
    src = (reverse) ? m_framebuf : src_end;
    dst = (reverse) ? src_end : m_framebuf;
    memmove(dst, src, n_bytes);

    for (i = 0; i < n_blank; i++) {
        int pos = (reverse) ? i : n_cells + i;
        vga_set_char(pos, BLANK_CHAR);
        vga_set_attr(pos, m_attr);
    }
}

static void erase(int mode)
{
    int start;
    int count;
    int pos = xy2pos(m_cursor.x, m_cursor.y);
    int area = m_rows * m_cols;

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
        vga_set_char(start + i, BLANK_CHAR);
        vga_set_attr(start + i, m_attr);
    }
}

static void erase_line(int mode)
{
    int start;
    int count;
    int pos = xy2pos(m_cursor.x, m_cursor.y);

    switch (mode) {
        case ERASE_DOWN:    // erase line from cursor down
            start = pos;
            count = m_cols - (pos % m_cols);
            break;
        case ERASE_UP:      // erase line from cursor up
            start = xy2pos(0, m_cursor.y);
            count = (pos % m_cols) + 1;
            break;
        case ERASE_ALL:     // erase entire line
        default:
            start = xy2pos(0, m_cursor.y);
            count = m_cols;
    }

    for (int i = 0; i < count; i++) {
        vga_set_char(start + i, BLANK_CHAR);
        vga_set_attr(start + i, m_attr);
    }
}

static void cursor_up(int n)
{
    if (m_cursor.y - n > 0) {
        m_cursor.y -= n;
    }
    else {
        m_cursor.y = 0;
    }
}

static void cursor_down(int n)
{
    if (m_cursor.y + n < m_rows - 1) {
        m_cursor.y += n;
    }
    else {
        m_cursor.y = m_rows - 1;
    }
}

static void cursor_left(int n)
{
    if (m_cursor.x - n > 0) {
        m_cursor.x -= n;
    }
    else {
        m_cursor.x = 0;
    }
}

static void cursor_right(int n)
{
    if (m_cursor.x + n < m_cols - 1) {
        m_cursor.x += n;
    }
    else {
        m_cursor.x = m_cols - 1;
    }
}

static void pos2xy(uint16_t pos, uint16_t *x, uint16_t *y)
{
    *x = pos % m_cols;
    *y = pos / m_cols;
}

static uint16_t xy2pos(uint16_t x, uint16_t y)
{
    return y * m_cols + x;
}

// ----------------------------------------------------------------------------

static void vga_set_char(uint16_t pos, char c)
{
    ((struct vga_cell *) m_framebuf)[pos].ch = c;
}

static void vga_set_attr(uint16_t pos, struct _char_attr attr)
{
    struct vga_attr *vga_attr;
    vga_attr = &((struct vga_cell *) m_framebuf)[pos].attr;

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

static void vga_enable_char_blink(void)
{
    uint32_t flags;
    cli_save(flags);

    uint8_t modectl;
    modectl = vga_attr_read(VGA_ATTR_REG_MODE);
    modectl |= VGA_ATTR_FLD_MODE_BLINK;
    vga_attr_write(VGA_ATTR_REG_MODE, modectl);

    restore_flags(flags);
}

static void vga_disable_char_blink(void)
{
    uint32_t flags;
    cli_save(flags);

    uint8_t modectl;
    modectl = vga_attr_read(VGA_ATTR_REG_MODE);
    modectl &= ~VGA_ATTR_FLD_MODE_BLINK;
    vga_attr_write(VGA_ATTR_REG_MODE, modectl);

    restore_flags(flags);
}

static void vga_show_cursor(void)
{
    uint32_t flags;
    cli_save(flags);

    uint8_t css;
    css = vga_crtc_read(VGA_CRTC_REG_CSS);
    css &= ~VGA_CRTC_FLD_CSS_CD_MASK;
    vga_crtc_write(VGA_CRTC_REG_CSS, css);

    restore_flags(flags);
}

static void vga_hide_cursor(void)
{
    uint32_t flags;
    cli_save(flags);

    uint8_t css;
    css = vga_crtc_read(VGA_CRTC_REG_CSS);
    css |= VGA_CRTC_FLD_CSS_CD_MASK;
    vga_crtc_write(VGA_CRTC_REG_CSS, css);

    restore_flags(flags);
}

static uint16_t vga_get_cursor(void)
{
    uint32_t flags;
    cli_save(flags);

    uint8_t poshi, poslo;
    poshi = vga_crtc_read(VGA_CRTC_REG_CL_HI);
    poslo = vga_crtc_read(VGA_CRTC_REG_CL_LO);

    restore_flags(flags);
    return (poshi << 8) | poslo;
}

static void vga_set_cursor(uint16_t pos)
{
    uint32_t flags;
    cli_save(flags);

    vga_crtc_write(VGA_CRTC_REG_CL_HI, pos >> 8);
    vga_crtc_write(VGA_CRTC_REG_CL_LO, pos & 0xFF);

    restore_flags(flags);
}

static uint16_t vga_get_cursor_shape(void)
{
    uint32_t flags;
    cli_save(flags);

    uint8_t shapehi, shapelo;
    shapelo = vga_crtc_read(VGA_CRTC_REG_CSS) & VGA_CRTC_FLD_CSS_CSS_MASK;
    shapehi = vga_crtc_read(VGA_CRTC_REG_CSE) & VGA_CRTC_FLD_CSE_CSE_MASK;

    restore_flags(flags);
    return (shapehi << 8) | shapelo;
}

static void vga_set_cursor_shape(uint8_t start, uint8_t end)
{
    uint32_t flags;
    cli_save(flags);

    vga_crtc_write(VGA_CRTC_REG_CSS, start & VGA_CRTC_FLD_CSS_CSS_MASK);
    vga_crtc_write(VGA_CRTC_REG_CSE, end   & VGA_CRTC_FLD_CSE_CSE_MASK);

    restore_flags(flags);
}
