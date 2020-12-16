/*============================================================================*
 * Copyright (C) 2020 Wes Hampson. All Rights Reserved.                       *
 *                                                                            *
 * This file is part of the Niobium Operating System.                         *
 * Niobium is free software; you may redistribute it and/or modify it under   *
 * the terms of the license agreement provided with this software.            *
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

#include <ctype.h>
#include <string.h>
#include <drivers/vga.h>
#include <nb/console.h>
#include <nb/input.h>

#define DEFAULT_FG          VGA_WHT
#define DEFAULT_BG          VGA_BLK
#define CURSOR_DEFAULT      0x0E0D
#define CURSOR_BLOCK        0x0F00
#define BLANK_CHAR          ' '

#define m_initialized   (consoles[curr_con].initialized)
#define m_cols          (consoles[curr_con].cols)
#define m_rows          (consoles[curr_con].rows)
#define m_framebuf      ((struct vga_cell *) consoles[curr_con].framebuf)
#define m_disp          (consoles[curr_con].disp)
#define m_attr          (consoles[curr_con].attr)
#define m_cursor        (consoles[curr_con].cursor)
#define m_state         (consoles[curr_con].state)

static struct console consoles[NUM_CONSOLES] = { 0 };
static int curr_con = 0;

static void defaults(struct console *con);
static void esc(char c);
static void bs(void);
static void cr(void);
static void lf(void);
static void r_lf(void);
static void scroll(int n);
static void set_vga_attr(struct vga_attr *a);
static void update_vga_attr(int pos);
static void cursor_up(int n);
static void cursor_down(int n);
static void cursor_left(int n);
static void cursor_right(int n);
static void cursor_home(void);
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
    m_initialized = true;
}

static void defaults(struct console *con)
{
    memset(con, 0, sizeof(struct console));
    con->cols = VGA_TEXT_COLS;
    con->rows = VGA_TEXT_ROWS;
    con->framebuf = (char *) VGA_FRAMEBUF_COLOR;
    con->attr.bg = DEFAULT_BG;
    con->attr.fg = DEFAULT_FG;
    con->cursor.shape = CURSOR_DEFAULT;
}

void con_write(char c)
{
    int pos = xy2pos(m_cursor.x, m_cursor.y);
    bool update_char = false;
    bool update_attr = false;
    bool update_curs = true;
    bool needs_crlf = false;

    if (iscntrl(c)) {
        goto cntrl;
    }

    switch (m_state)
    {
        case S_ESC:
            esc(c);
            break;
        
        // case S_CSI:
        //     csi(c);
        //     break;
    
    cntrl:
        default:
            switch (c)
            {
                case ASCII_ESC:
                    m_state = S_ESC;
                    return;
                case ASCII_BS:      /* backspace */
                    if (m_cursor.x > 0) {
                        pos--;
                    }
                    bs();
                    c = BLANK_CHAR;
                    update_char = true;
                    update_attr = true;
                    break;
                case ASCII_LF:      /* line feed */
                case ASCII_VT:      /* vertical tab */
                case ASCII_FF:      /* form feed */
                    lf();
                    break;
                case ASCII_CR:      /* carriage return */
                    cr();
                    break;
                default:
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
}

static void esc(char c)
{
    /* Some VT52/VT100/Linux escape sequences, not 100% compliant */
    /* TODO: difference between line feed and cursor down? */

    switch (c) {
        case 'A':   /* cursor up */
            cursor_up(1);
            break;
        case 'B':   /* cursor down */
            cursor_down(1);
            break;
        case 'C':   /* cursor right */
            cursor_right(1);
            break;
        case 'D':   /* cursor left */
            cursor_left(1);
            break;
        case 'E':   /* CRLF */
            cr(); lf();
            break;
        case 'H':   /* cursor 0,0 */
            cursor_home();
            break;
        case 'I':   /* reverse line feed */
            r_lf();
            break;
        // case 'J':   /* erase to end of screen */
        // case 'K':   /* erase to end of line */
        // case 'c':   /* reset console */
        /* TODO: more? can even come up with my own! :D */
        default:
            break;
    }

    m_state = S_NORM;
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
    if (m_attr.blink && m_disp.blink_on) {
        a->blink = 1;
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

static void cursor_home(void)
{
    m_cursor.x = 0;
    m_cursor.y = 0;
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
