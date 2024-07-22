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
#include <console.h>
#include <ctype.h>
#include <errno.h>
#include <irq.h>
#include <kernel.h>
#include <ps2.h>
#include <vga.h>
#include <fs.h>
#include <io.h>

#define NUM_CONSOLES    1

#define BEEP_FREQUENCY  750     // Hz
#define BEEP_DURATION   50      // ms

static struct console consoles[NUM_CONSOLES] = { };
static int active_cons = 0;

static char _input_buffers[NUM_CONSOLES][INPUT_BUFFER_SIZE];

//
// This is totally something I yoinked from Linux. It allows for n consoles and
// uses clever yet slightly evil macros which allow member access to the active
// console in the console array. I don't know if I'll keep it but for now it'll
// do.
//
#define m_initialized   (consoles[active_cons].initialized)
#define m_cols          (consoles[active_cons].cols)
#define m_rows          (consoles[active_cons].rows)
#define m_framebuf      (consoles[active_cons].framebuf)
#define m_inputq        (&consoles[active_cons].inputq)
#define m_tabstop       (consoles[active_cons].tabstop)
#define m_csiparam      (consoles[active_cons].csiparam)
#define m_paramidx      (consoles[active_cons].paramidx)
#define m_currparam     (m_csiparam[m_paramidx])
#define m_disp          (consoles[active_cons].disp)
#define m_attr          (consoles[active_cons].attr)
#define m_attr_default  (consoles[active_cons].attr_default)
#define m_cursor        (consoles[active_cons].cursor)
#define m_saved         (consoles[active_cons].saved)
#define m_defaults      (consoles[active_cons].defaults)
#define m_state         (consoles[active_cons].state)

#define CURSOR_SHAPE_UNDERLINE  0x0E0D
#define CURSOR_SHAPE_BLOCK      0x0F00
#define BLANK                   ' '
#define DEFAULT_CSIPARAM        (-1)

static void defaults(struct console *cons);
static void save_state(void);
static void restore_state(void);
static void reset(void);
static void esc(char c);
static void csi(char c);
static void csi_m(char p);
static void bs(void);
static void cr(void);
static void lf(void);
static void r_lf(void);
static void tab(void);
static void scroll(int n);
static void erase(int mode);
static void erase_ln(int mode);
static void cursor_up(int n);
static void cursor_down(int n);
static void cursor_left(int n);
static void cursor_right(int n);
static void cursor_save(void);
static void cursor_restore(void);
static void read_cursor(void);
static void write_cursor(void);
static void pos2xy(int pos, int *x, int *y);
static int  xy2pos(int x, int y);
static void set_vga_char(int pos, char c);
static void set_vga_attr(int pos, const struct char_attr *attr);
static void _writestr(char *str);
static void _dowrite(char c);
static void drain_kb(void);


void init_console(void)
{
    zeromem(_input_buffers, sizeof(_input_buffers));
    zeromem(consoles, sizeof(consoles));

    for (int i = 0; i < NUM_CONSOLES; i++) {
        defaults(&consoles[i]);
        char_queue_init(&consoles[i].inputq, _input_buffers[i], INPUT_BUFFER_SIZE);
    }

    irq_register(IRQ_TIMER, drain_kb);

    active_cons = 0;
    read_cursor();
    save_state();
    _writestr("\r\n");
    m_initialized = true;
}

static void defaults(struct console *cons)
{
    cons->cols = VGA_COLS;
    cons->rows = VGA_ROWS;
    cons->framebuf = (void *) VGA_FRAMEBUF;
    cons->disp.blink_on = false;
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
    cons->cursor.shape = CURSOR_SHAPE_UNDERLINE;
    cons->cursor.hidden = false;
    cons->defaults.attr = cons->attr;
    cons->defaults.cursor = cons->cursor;
    cons->state = S_NORM;
    cons->paramidx = 0;
    memset(cons->csiparam, DEFAULT_CSIPARAM, MAX_CSIPARAMS);
    for (int i = 0; i < MAX_TABSTOPS; i++) {
        m_tabstop[i] = (((i + 1) % TABSTOP_WIDTH) == 0);
    }
}

int console_read(struct file *file, char *buf, size_t count)
{
    int nread;
    uint32_t flags;
    (void) file;

    if (!buf) {
        return -EINVAL;
    }

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

int console_write(struct file *file, const char *buf, size_t count)
{
    (void) file;

    if (!buf) {
        return -EINVAL;
    }

    for (int i = 0; i < count; i++) {
        _dowrite(buf[i]);
#if E9_HACK
        outb(0xE9, buf[i]);
#endif
    }

    return count;
}

static void drain_kb(void)
{
    int c;

    while ((c = kb_read()) != -1) {
        if (!char_queue_full(m_inputq)) {
            char_queue_put(m_inputq, (char) c);
        }
        else {
            kprint("console: input buffer full!\n");
            beep(1000, 50);
        }
    }
}

static void _dowrite(char c)
{
    uint32_t flags;
    cli_save(flags);

    read_cursor();
    int pos = xy2pos(m_cursor.x, m_cursor.y);

    bool update_char = false;
    bool update_attr = false;
    bool update_cursor = true;
    bool need_crlf = false;

    // ONLCR conversion (newline to carriage return-newline)
    if (c == '\n') cr();

    // handle escape sequences if not a control character
    if (!iscntrl(c)) {
        switch (m_state) {
            case S_ESC:
                esc(c);
                goto update;
            case S_CSI:
                csi(c);
                goto update;
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
            beep(BEEP_FREQUENCY, BEEP_DURATION);
            break;
        case '\b':      // ^H - BS - backspace
            if (m_cursor.x > 0) pos--;
            bs();
            __fallthrough;
        case ASCII_DEL: // ^? - DEL - delete
            c = BLANK;
            update_char = true;
            update_attr = true;
            break;
        case '\t':      // ^I - HT - horizontal tab
            tab();
            break;
        case '\n':      // ^J - LF - line feed
        case '\v':      // ^K - VT - vertical tab
        case '\f':      // ^L - FF - form feed
            lf();
            break;
        case '\r':      // ^M - CR -  carriage return
            cr();
            break;
        case ASCII_CAN: // ^X - CAN - cancel escape sequence
            m_state = S_NORM;
            goto done;
        case '\e':      // ^[ - ESC - start escape sequence
            m_state = S_ESC;
            goto done;

    // everything else
        default:
            if (iscntrl(c)) {
                goto done;
            }
            update_char = true;
            update_attr = true;
            if (++m_cursor.x >= m_cols) {
                need_crlf = true;
            }
            break;
    }

update:
    if (update_char) {
        set_vga_char(pos, c);
    }
    if (update_attr) {
        if (m_attr.bright && m_attr.faint) {
            m_attr.bright = false;
        }
        set_vga_attr(pos, &m_attr);
    }
    if (need_crlf) {
        cr(); lf();
    }
    if (update_cursor) {
        write_cursor();
    }

done:
    restore_flags(flags);
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
            cr(); lf();
            break;
        case 'E':       // ESC E - NEL - newline (CRLF)
            cr(); lf();
            break;
        case 'H':       // ESC H - HTS - set tab stop
            m_tabstop[m_cursor.x] = 1;
            break;
        case 'M':       // ESC M - RI - reverse line feed
            r_lf();
            break;
        case '[':       // ESC [ - CSI - control sequence introducer
            memset(m_csiparam, DEFAULT_CSIPARAM, MAX_CSIPARAMS);
            m_paramidx = 0;
            m_state = S_CSI;
            return;

        //
        // "Custom" console-related sequences
        //
        case '3':       // ESC 3    disable blink
            m_disp.blink_on = false;
            vga_disable_blink();
            break;
        case '4':       // ESC 4    enable blink
            m_disp.blink_on = true;
            vga_enable_blink();
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
            save_state();
            break;
        case '8':       // ESC 8    restore console
            restore_state();
            break;
        case 'c':       // ESC c    reset console
            reset();
            break;
        case 'h':       // ESC h    clear tab stop
            m_tabstop[m_cursor.x] = 0;
            break;
        default:
            break;
    }

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

    int i = 0;
    switch (c)
    {
        //
        // "Standard" sequences
        //
        case 'A':       // CSI n A  - CUU - move cursor up n rows
            param_minimum(0, 1);
            cursor_up(m_csiparam[0]);
            break;
        case 'B':       // CSI n B  - CUD - move cursor down n rows
            param_minimum(0, 1);
            cursor_down(m_csiparam[0]);
            break;
        case 'C':       // CSI n C  - CUF - move cursor right (forward) n columns
            param_minimum(0, 1);
            cursor_right(m_csiparam[0]);
            break;
        case 'D':       // CSI n D  - CUB - move cursor left (back) n columns
            param_minimum(0, 1);
            cursor_left(m_csiparam[0]);
            break;
        case 'E':       // CSI n E  - CNL - move cursor to beginning of line, n rows down
            param_minimum(0, 1);
            m_cursor.x = 0;
            cursor_down(m_csiparam[0]);
            break;
        case 'F':       // CSI n F  - CPL - move cursor to beginning of line, n rows up
            param_minimum(0, 1);
            m_cursor.x = 0;
            cursor_up(m_csiparam[0]);
            break;
        case 'G':       // CSI n G  - CHA - move cursor to column n
            param_minimum(0, 1);
            if (m_csiparam[0] > m_cols) m_csiparam[0] = m_cols;
            m_cursor.x = m_csiparam[0] - 1;
            break;
        case 'H':       // CSI n ; m H - CUP - move cursor row n, column m
            param_minimum(0, 1);
            param_minimum(1, 1);
            if (m_csiparam[0] > m_rows) m_csiparam[0] = m_rows;
            if (m_csiparam[1] > m_cols) m_csiparam[1] = m_cols;
            m_cursor.y = m_csiparam[0] - 1;
            m_cursor.x = m_csiparam[1] - 1;
            break;
        case 'J':       // CSI n J  - ED - erase in display (n = mode)
            param_minimum(0, 0);
            erase(m_csiparam[0]);
            break;
        case 'K':       // CSI n K  - EL- erase in line (n = mode)
            param_minimum(0, 0);
            erase_ln(m_csiparam[0]);
            break;
        case 'S':       // CSI n S  - SU - scroll n lines
            param_minimum(0, 1);
            scroll(m_csiparam[0]);
            break;
        case 'T':       // CSI n T  - ST - reverse scroll n lines
            param_minimum(0, 1);
            scroll(-m_csiparam[0]);
            break;
        case 'm':       // CSI n m  - SGR - set graphics attribute
            param_minimum(0, 0);
            while (i <= m_paramidx) csi_m(m_csiparam[i++]);
            break;

        //
        // Custom (or "private") sequences
        //
        case 's':       // CSI s        save cursor position
            cursor_save();
            break;
        case 'u':       // CSI u        restore cursor position
            cursor_restore();
            break;

        //
        // CSI params
        //
        case ';':       // (parameter separator)
            if (++m_paramidx >= MAX_CSIPARAMS) {
                break;
            }
            return;
        default:        // (parameter)
            if (isdigit(c)) {
                if (m_currparam == DEFAULT_CSIPARAM) {
                    m_currparam = 0;
                }
                m_currparam *= 10;
                m_currparam += (c - '0');
                return;
            }
            break;
    }

    m_state = S_NORM;

    #undef param_minimum
}

static void csi_m(char p)
{
    static const char CSI_COLORS[8] =
    {
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
            m_attr = m_defaults.attr;
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
            if (p >= 30 && p <= 37) m_attr.fg = CSI_COLORS[p - 30];     // yeah yeah this is ugly but it works for now
            if (p >= 40 && p <= 47) m_attr.bg = CSI_COLORS[p - 40];
            if (p == 39) m_attr.fg = m_defaults.attr.fg;
            if (p == 49) m_attr.bg = m_defaults.attr.bg;
            if (p >= 90 && p <= 97) {
                m_attr.fg = CSI_COLORS[p - 90];
                m_attr.bright = 1;
            }
            if (p >= 100 && p <= 107) {
                m_attr.bg = CSI_COLORS[p - 100];
                m_attr.bright = !m_attr.blink;
            }
            break;
    }
}

static void save_state(void)
{
    m_saved.disp = m_disp;
    m_saved.attr = m_attr;
    cursor_save();
    memcpy(&m_saved.tabstop, &m_tabstop, MAX_TABSTOPS);
}

static void restore_state(void)
{
    m_disp = m_saved.disp;
    m_attr = m_saved.attr;
    cursor_restore();
    memcpy(&m_tabstop, &m_saved.tabstop, MAX_TABSTOPS);
}

static void reset(void)
{
    defaults(&consoles[active_cons]);
    write_cursor();
    erase(ERASE_ALL);
}

static void bs(void)
{
    if (--m_cursor.x < 0) {
        m_cursor.x = 0;
    }
}

static void cr(void)
{
    m_cursor.x = 0;
}

static void lf(void)
{
    if (++m_cursor.y >= m_rows) {
        scroll(1);
        m_cursor.y--;
    }
}

static void r_lf(void)
{
    if (--m_cursor.y < 0) {
        scroll(-1);
        m_cursor.y++;
    }
}

static void tab(void)
{
    while (m_cursor.x < m_cols) {
        if (m_tabstop[++m_cursor.x]) {
            break;
        }
    }

    if (m_cursor.x >= m_cols) {
        m_cursor.x = m_cols - 1;
    }
}

static void scroll(int n)
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
        set_vga_char(pos, BLANK);
        set_vga_attr(pos, &m_attr);
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
        set_vga_char(start + i, BLANK);
        set_vga_attr(start + i, &m_attr);
    }
}

static void erase_ln(int mode)
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
        set_vga_char(start + i, BLANK);
        set_vga_attr(start + i, &m_attr);
    }
}

static void cursor_up(int n)
{
    m_cursor.y -= n;
    if (m_cursor.y < 0) {
        m_cursor.y = 0;
    }
}

static void cursor_down(int n)
{
    m_cursor.y += n;
    if (m_cursor.y >= m_rows) {
        m_cursor.y = m_rows - 1;
    }
}

static void cursor_left(int n)
{
    m_cursor.x -= n;
    if (m_cursor.x < 0) {
        m_cursor.x = 0;
    }
}

static void cursor_right(int n)
{
    m_cursor.x += n;
    if (m_cursor.x >= m_cols) {
        m_cursor.x = m_cols - 1;
    }
}

static void cursor_save(void)
{
    m_saved.cursor = m_cursor;
}

static void cursor_restore(void)
{
    m_cursor = m_saved.cursor;
}

static void read_cursor(void)
{
    m_cursor.shape = vga_get_cursor_shape();
    pos2xy(vga_get_cursor_pos(), &m_cursor.x, &m_cursor.y);
}

static void write_cursor(void)
{
    vga_set_cursor_pos(xy2pos(m_cursor.x, m_cursor.y));
    if (!m_cursor.hidden) {
        vga_set_cursor_shape(m_cursor.shape & 0xFF, m_cursor.shape >> 8);
    }
}

static inline void pos2xy(int pos, int *x, int *y)
{
    *x = pos % m_cols;
    *y = pos / m_cols;
}

static inline int xy2pos(int x, int y)
{
    return y * m_cols + x;
}

static void set_vga_char(int pos, char c)
{
    struct vga_cell *cell;
    cell = (struct vga_cell *) m_framebuf;

    cell[pos].ch = c;
}

static void set_vga_attr(int pos, const struct char_attr *attr)
{
    struct vga_attr *vga_attr;
    vga_attr = &((struct vga_cell *) m_framebuf)[pos].attr;

    vga_attr->bg = attr->bg;
    vga_attr->fg = attr->fg;

    if (attr->bright) {
        vga_attr->bright = 1;
    }
    if (attr->faint) {
        vga_attr->color_fg = VGA_BLACK;  // simulate faintness with dark gray
        vga_attr->bright = 1;
    }
    if (attr->underline) {
        vga_attr->color_fg = VGA_CYAN;   // simulate underline with cyan
        vga_attr->bright = attr->bright;
    }
    if (attr->italic) {
        vga_attr->color_fg = VGA_GREEN;  // simulate italics with green
        vga_attr->bright = attr->bright;
    }
    if (attr->blink) {
        vga_attr->blink = 1;
    }
    if (attr->invert) {
        swap(vga_attr->color_bg, vga_attr->color_fg);
    }
}

static void _writestr(char *str)
{
    while (*str != '\0') {
        _dowrite(*str++);
    }
}
