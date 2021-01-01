/*============================================================================*
 * Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                  *
 *                                                                            *
 * This file is part of the OHWES Operating System.                           *
 * OHWES is free software; you may redistribute it and/or modify it under the *
 * terms of the license agreement provided with this software.                *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: kernel/console.c                                                  *
 * Created: December 13, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#include <ascii.h>
#include <ctype.h>
#include <string.h>
#include <drivers/vga.h>
#include <ohwes/console.h>
#include <ohwes/kernel.h>
#include <ohwes/interrupt.h>
#include <ohwes/ohwes.h>

#define DEFAULT_FG          VGA_WHT
#define DEFAULT_BG          VGA_BLK
#define CURSOR_DEFAULT      0x0E0D
#define CURSOR_BLOCK        0x0F00
#define BLANK_CHAR          ' '
#define CSIPARAM_DEFAULT    (-1)

#define m_initialized   (consoles[curr_con].initialized)
#define m_cols          (consoles[curr_con].cols)
#define m_rows          (consoles[curr_con].rows)
#define m_tabsize       (consoles[curr_con].tabsize)
#define m_framebuf      ((struct vga_cell *) consoles[curr_con].framebuf)
#define m_disp          (consoles[curr_con].disp)
#define m_attr          (consoles[curr_con].attr)
#define m_attr_default  (consoles[curr_con].attr_default)
#define m_cursor        (consoles[curr_con].cursor)
#define m_saved         (consoles[curr_con].saved)
#define m_defaults      (consoles[curr_con].defaults)
#define m_state         (consoles[curr_con].state)
#define m_csiparam      (consoles[curr_con].csiparam)
#define m_paramidx      (consoles[curr_con].paramidx)
#define m_currparam     (m_csiparam[m_paramidx])

static struct console consoles[NUM_CONSOLES] = { 0 };
static int curr_con = 0;

static const char CSI_COLORS[8] =
{
    VGA_BLK,
    VGA_RED,
    VGA_GRN,
    VGA_BRN,
    VGA_BLU,
    VGA_MGT,
    VGA_CYN,
    VGA_WHT
};

static void defaults(struct console *con);
static void reset(void);
static void save(void);
static void restore(void);
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
static void set_vga_attr(struct vga_attr *a);
static void update_vga_attr(int pos);
static void cursor_up(int n);
static void cursor_down(int n);
static void cursor_left(int n);
static void cursor_right(int n);
static void update_cursor(void);
static void pos2xy(int pos, int *x, int *y);
static int xy2pos(int x, int y);

void con_init(void)
{
    vga_init();

    curr_con = 0;
    defaults(&consoles[curr_con]);
    m_cursor.shape = vga_get_cursor_shape();
    pos2xy(vga_get_cursor_pos(), &m_cursor.x, &m_cursor.y);
    cr(); lf();
    m_initialized = true;
}

static void reset(void)
{
    defaults(&consoles[curr_con]);
    erase(0);
    update_cursor();
}

static void defaults(struct console *con)
{
    memset(con, 0, sizeof(struct console));
    memset(con->csiparam, CSIPARAM_DEFAULT, MAX_CSIPARAMS);
    con->cols = VGA_TEXT_COLS;
    con->rows = VGA_TEXT_ROWS;
    con->tabsize = 8;
    con->framebuf = (char *) VGA_FRAMEBUF_COLOR;
    con->attr.bg = DEFAULT_BG;
    con->attr.fg = DEFAULT_FG;
    con->cursor.shape = CURSOR_DEFAULT;
    con->defaults.attr = con->attr;
    con->defaults.cursor = con->cursor;
}

void con_write(char c)
{
    uint32_t flags;

    cli_save(flags);
    int pos = xy2pos(m_cursor.x, m_cursor.y);
    bool update_char = false;
    bool update_attr = false;
    bool update_curs = true;
    bool needs_crlf = false;

    /* CRNL conversion */
    /* TODO: move this elsewhere, along with other output formatting */
    if (c == '\n') cr();

    if (iscntrl(c)) {
        goto cntrl;
    }

    switch (m_state)
    {
        case S_ESC:
            esc(c);
            break;

        case S_CSI:
            csi(c);
            break;

    cntrl:
        default:
            switch (c)
            {
                case ASCII_BEL:     /* ^G   beep! */
                    /*beep();*/
                    break;
                case ASCII_BS:      /* ^H   backspace char */
                    if (m_cursor.x > 0) pos--;
                    bs();
                    c = BLANK_CHAR;
                    update_char = true;
                    update_attr = true;
                    break;
                case ASCII_HT:      /* ^I   horizonal tab */
                    tab();
                    break;
                case ASCII_LF:      /* ^J   line feed */
                case ASCII_VT:      /* ^K   vertical tab */
                case ASCII_FF:      /* ^L   form feed */
                    lf();
                    break;
                case ASCII_CR:      /* ^M   carriage return */
                    cr();
                    break;
                case ASCII_CAN:     /* ^X   abort escape sequence */
                    m_state = S_NORM;
                    goto done;
                case ASCII_ESC:     /* ^[   start escape sequence */
                    m_state = S_ESC;
                    goto done;
                case ASCII_DEL:     /* ^?   delete char */
                    c = BLANK_CHAR;
                    update_char = true;
                    update_attr = true;
                    break;
                default:
                    if (iscntrl(c)) {
                        goto done;
                    }
                    update_char = true;
                    update_attr = true;
                    if (++m_cursor.x >= m_cols) {
                        needs_crlf = true;
                    }
                    break;
            }
    }

    if (update_char) {
        m_framebuf[pos].ch = c;
    }
    if (update_attr) {
        update_vga_attr(pos);
    }
    if (needs_crlf) {
        cr(); lf();
    }
    if (update_curs) {
        update_cursor();
    }

done:
    restore_flags(flags);
}

static void esc(char c)
{
    /* Some VT52/VT100 escape sequences, not 100% compliant */

    switch (c) {
        case 'A':       /* ^[A  move cursor up */
            cursor_up(1);
            break;
        case 'B':       /* ^[B  move cursor down */
            cursor_down(1);
            break;
        case 'C':       /* ^[C  move cursor right */
            cursor_right(1);
            break;
        case 'D':       /* ^[D  move cursor left */
            cursor_left(1);
            break;
        case 'E':       /* ^[E  newline (CRLF) */
            cr(); lf();
            break;
        case 'H':       /* ^[H  set cursor to row 1, column 1 */
            m_cursor.x = 0;
            m_cursor.y = 0;
            break;
        case 'I':       /* ^[I  reverse line feed */
            r_lf();
            break;
        case 'J':       /* ^[J  erase to end of screen */
            erase(0);
            break;
        case 'K':       /* ^[K  erase to end of line */
            erase_ln(0);
            break;
        case 'M':       /* ^[M  line feed */
            lf();
            break;
        case 'c':       /* ^[c  reset console */
            reset();
            break;
        case '3':       /* ^[3  disable blink */
            m_disp.blink_on = false;
            vga_disable_blink();
            break;
        case '4':       /* ^[4  enable blink */
            m_disp.blink_on = true;
            vga_enable_blink();
            break;
        case '5':       /* ^[5  hide cursor */
            m_cursor.hidden = true;
            vga_hide_cursor();
            break;
        case '6':       /* ^[6  show cursor */
            m_cursor.hidden = false;
            vga_show_cursor();
            break;
        case '7':       /* ^[7  save state */
            save();
            break;
        case '8':       /* ^[8  restore state */
            restore();
            break;
        case '[':       /* ^[[  control sequence introducer */
            memset(m_csiparam, CSIPARAM_DEFAULT, MAX_CSIPARAMS);
            m_paramidx = 0;
            m_state = S_CSI;
            return;
        default:
            break;
    }

    m_state = S_NORM;
}

static void csi(char c)
{
    /* Some VT52/VT100 control sequences, not 100% compliant */

    int i = 0;
    switch (c)
    {
        case 'A':       /* ^[[nA    move cursor up n rows */
            if (m_csiparam[0] < 1) m_csiparam[0] = 1;
            cursor_up(m_csiparam[0]);
            m_state = S_NORM;
            break;
        case 'B':       /* ^[[nB    cursor down n rows */
            if (m_csiparam[0] < 1) m_csiparam[0] = 1;
            cursor_down(m_csiparam[0]);
            m_state = S_NORM;
            break;
        case 'C':       /* ^[[nC    cursor right n columns */
            if (m_csiparam[0] < 1) m_csiparam[0] = 1;
            cursor_right(m_csiparam[0]);
            m_state = S_NORM;
            break;
        case 'D':       /* ^[[nD    cursor left n columns */
            if (m_csiparam[0] < 1) m_csiparam[0] = 1;
            cursor_left(m_csiparam[0]);
            m_state = S_NORM;
            break;
        case 'E':       /* ^[[nE    n newlines (CRLF) */
            if (m_csiparam[0] < 1) m_csiparam[0] = 1;
            for (i = 0; i < m_csiparam[0]; i++) {
                cr(); lf();
            }
            m_state = S_NORM;
            break;
        case 'H':       /* ^[[m;nH  set cursor row m, column n */
        case 'f':       /* ^[[m;nf  set cursor row m, comumn n */
            if (m_csiparam[0] < 1) m_csiparam[0] = 1;
            if (m_csiparam[1] < 1) m_csiparam[1] = 1;
            if (m_csiparam[0] > m_rows) m_csiparam[0] = m_rows;
            if (m_csiparam[1] > m_cols) m_csiparam[1] = m_cols;
            m_cursor.y = m_csiparam[0] - 1;
            m_cursor.x = m_csiparam[1] - 1;
            m_state = S_NORM;
            break;
        case 'I':       /* ^[[nI    n reverse linefeeds */
            if (m_csiparam[0] < 1) m_csiparam[0] = 1;
            for (i = 0; i < m_csiparam[0]; i++) {
                r_lf();
            }
            m_state = S_NORM;
            break;
        case 'J':       /* ^[[mJ    erase screen */
            if (m_csiparam[0] < 0) m_csiparam[0] = 0;
            erase(m_csiparam[0]);
            m_state = S_NORM;
            break;
        case 'K':       /* ^[[mK    erase current line */
            if (m_csiparam[0] < 0) m_csiparam[0] = 0;
            erase_ln(m_csiparam[0]);
            m_state = S_NORM;
            break;
        case 'M':       /* ^[[nM    n linefeeds */
            if (m_csiparam[0] < 1) m_csiparam[0] = 1;
            for (i = 0; i < m_csiparam[0]; i++) {
                lf();
            }
            m_state = S_NORM;
            break;
        case 'm':       /* ^[[xm    set graphics attribute */
            while (i <= m_paramidx) {
                csi_m(m_csiparam[i++]);
            }
            m_state = S_NORM;
            break;
        case ';':       /* (parameter separator) */
            if (++m_paramidx >= MAX_CSIPARAMS) {
                m_state = S_NORM;
            }
            break;
        default:        /* (parameter) */
            if (isdigit(c)) {
                if (m_currparam == CSIPARAM_DEFAULT) {
                    m_currparam = 0;
                }
                m_currparam *= 10;  /* next digit */
                m_currparam += (c - '0');
            }
            else {
                m_state = S_NORM;
            }
            break;
    }
}

static void csi_m(char p)
{
    /* CSIm set character attribute */

    switch (p) {
        case 0:
            m_attr = m_defaults.attr;
            break;
        case 1:
            m_attr.bright = true;
            break;
        case 2:
            m_attr.faint = true;
            break;
        case 4:
            m_attr.underline = true;
            break;
        case 5:
            m_attr.blink = true;
            break;
        case 7:
            m_attr.invert = true;
            break;
        case 21:
            m_attr.bright = false;
            break;
        case 22:
            m_attr.faint = false;
            break;
        case 24:
            m_attr.underline = false;
            break;
        case 25:
            m_attr.blink = false;
            break;
        case 27:
            m_attr.invert = false;
            break;
        default:
            if (p >= 30 && p <= 37) m_attr.fg = CSI_COLORS[p - 30];
            if (p >= 40 && p <= 47) m_attr.bg = CSI_COLORS[p - 40];
            if (p == 38 || p == 39) m_attr.fg = m_defaults.attr.fg;
            if (p == 48 || p == 49) m_attr.bg = m_defaults.attr.bg;
            break;
    }
}

static void save(void)
{
    m_saved.cursor = m_cursor;
    m_saved.attr = m_attr;
}

static void restore(void)
{
    m_cursor = m_saved.cursor;
    m_attr = m_saved.attr;
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
    char tmp;

    tmp = m_cursor.x % m_tabsize;
    if (tmp == 0) {
        m_cursor.x += m_tabsize;
    }
    else {
        m_cursor.x += (m_tabsize - tmp);
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
    struct vga_cell cell;
    void *src;
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

    src = (reverse) ? m_framebuf : &(m_framebuf[n_blank]);
    dst = (reverse) ? &(m_framebuf[n_blank]) : m_framebuf;
    memmove(dst, src, n_bytes);

    cell.ch = BLANK_CHAR;
    set_vga_attr(&cell.attr);
    for (i = 0; i < n_blank; i++) {
        m_framebuf[(reverse)? i: n_cells+i] = cell;
    }
}

static void erase(int mode)
{
    struct vga_cell *start;
    int count;
    int pos = xy2pos(m_cursor.x, m_cursor.y);
    int area = m_rows * m_cols;

    switch (mode) {
        case 0:     /* erase screen from cursor down */
            start = &m_framebuf[pos];
            count = area - pos;
            break;
        case 1:     /* erase screen from cursor up /*/
            start = m_framebuf;
            count = pos + 1;
            break;
        case 2:     /* erase entire screen */
            start = m_framebuf;
            count = area;
    }

    for (int i = 0; i < count; i++) {
        start[i].ch = BLANK_CHAR;
        set_vga_attr(&start[i].attr);
    }
}

static void erase_ln(int mode)
{
    struct vga_cell *start;
    int count;
    int pos = xy2pos(m_cursor.x, m_cursor.y);
    int area = m_cols;

    switch (mode) {
        case 0:     /* erase line from cursor down */
            start = &m_framebuf[pos];
            count = area - (pos % m_cols);
            break;
        case 1:     /* erase line from cursor up /*/
            start = m_framebuf;
            count = (pos % m_cols) + 1;
            break;
        case 2:     /* erase entire line */
            start = m_framebuf;
            count = area;
    }

    for (int i = 0; i < count; i++) {
        start[i].ch = BLANK_CHAR;
        set_vga_attr(&start[i].attr);
    }
}

static void update_vga_attr(int pos)
{
    set_vga_attr(&m_framebuf[pos].attr);
}

static void set_vga_attr(struct vga_attr *a)
{
    a->bg = m_attr.bg;
    a->fg = m_attr.fg;

    if (m_attr.bright) {
        a->bright = 1;
    }
    if (m_attr.faint) {
        a->fg = VGA_BLK;    /* emulate with dark gray */
        a->bright = 1;
    }
    if (m_attr.underline) {
        a->fg = VGA_CYN;    /* emulate with bright cyan */
        a->bright = 1;
    }
    if (m_attr.blink) {
        a->blink = 1;
    }
    if (m_attr.invert) {
        swap(a->color_bg, a->color_fg);
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

static void update_cursor(void)
{
    vga_set_cursor_pos(xy2pos(m_cursor.x, m_cursor.y));
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
