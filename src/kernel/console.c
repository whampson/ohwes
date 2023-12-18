// /* =============================================================================
//  * Copyright (C) 2020-2023 Wes Hampson. All Rights Reserved.
//  *
//  * This file is part of the OH-WES Operating System.
//  * OH-WES is free software; you may redistribute it and/or modify it under the
//  * terms of the GNU GPLv2. See the LICENSE file in the root of this repository.
//  *
//  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  * SOFTWARE.
//  * -----------------------------------------------------------------------------
//  *         File: kernel/console.c
//  *      Created: March 26, 2023
//  *       Author: Wes Hampson
//  * =============================================================================
//  */

// #include <sys/console.h>
// #include <sys/vga.h>

// char * const g_VgaBuf = (char * const) 0xB8000;

// uint16_t console_get_cursor()
// {
//     uint8_t cursorLocHi, cursorLocLo;
//     cursorLocHi = vga_crtc_read(VGA_REG_CRTC_CL_HI);
//     cursorLocLo = vga_crtc_read(VGA_REG_CRTC_CL_LO);

//     return (cursorLocHi << 8) | cursorLocLo;
// }

// void console_set_cursor(uint16_t pos)
// {
//     vga_crtc_write(VGA_REG_CRTC_CL_HI, pos >> 8);
//     vga_crtc_write(VGA_REG_CRTC_CL_LO, pos & 0xFF);
// }

// void console_write(char c)
// {
//     uint16_t pos = console_get_cursor();

//     switch (c)
//     {
//         case '\r':
//             pos -= (pos % 80);
//             break;
//         case '\n':
//             pos += (80 - (pos % 80));   // TODO: CRLF vs LF
//             break;

//         default:
//             g_VgaBuf[(pos++) << 1] = c;
//             break;
//     }

//     console_set_cursor(pos);
// }

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

#ifndef __ASCII_H
#define __ASCII_H

/**
 * ASCII Control Characters
 */
enum ascii_cntl
{
    ASCII_NUL,          /* Null */
    ASCII_SOH,          /* Start of Heading */
    ASCII_STX,          /* Start of Text */
    ASCII_ETX,          /* End of Text */
    ASCII_EOT,          /* End of Transmission */
    ASCII_ENQ,          /* Enquiry */
    ASCII_ACK,          /* Acknowledgement */
    ASCII_BEL,          /* Bell */
    ASCII_BS,           /* Backspace */
    ASCII_HT,           /* Horizontal Tab */
    ASCII_LF,           /* Line Feed */
    ASCII_VT,           /* Vertical Tab */
    ASCII_FF,           /* Form Feed */
    ASCII_CR,           /* Carriage Return */
    ASCII_SO,           /* Shift Out */
    ASCII_SI,           /* Shift In */
    ASCII_DLE,          /* Data Link Escape */
    ASCII_DC1,          /* Device Control 1 (XON) */
    ASCII_DC2,          /* Device Control 2 */
    ASCII_DC3,          /* Device Control 3 (XOFF) */
    ASCII_DC4,          /* Device Control 4 */
    ASCII_NAK,          /* Negative Acknowledgement */
    ASCII_SYN,          /* Synchronous Idle */
    ASCII_ETB,          /* End of Transmission Block */
    ASCII_CAN,          /* Cancel */
    ASCII_EM,           /* End of Medium */
    ASCII_SUB,          /* Substitute */
    ASCII_ESC,          /* Escape */
    ASCII_FS,           /* File Separator */
    ASCII_GS,           /* Group Separator */
    ASCII_RS,           /* Record Separator */
    ASCII_US,           /* Unit Separator */
    ASCII_DEL = 0x7F    /* Delete */
};

#endif /* __ASCII_H */

// #include <ascii.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/vga.h>
#include <sys/console.h>
// #include <sys/kernel.h>
#include <sys/interrupt.h>
// #include <sys/ohwes.h>

#define DEFAULT_FG          VGA_WHT
#define DEFAULT_BG          VGA_BLK
#define CURSOR_DEFAULT      0x0E0D
#define CURSOR_BLOCK        0x0F00
#define BLANK_CHAR          ' '
#define CSIPARAM_DEFAULT    (-1)

#define m_initialized   (consoles[curr_con].initialized)
#define m_cols          (consoles[curr_con].cols)
#define m_rows          (consoles[curr_con].rows)
#define m_framebuf      ((struct vga_cell *) consoles[curr_con].framebuf)
#define m_tabstop       (consoles[curr_con].tabstop)
#define m_csiparam      (consoles[curr_con].csiparam)
#define m_paramidx      (consoles[curr_con].paramidx)
#define m_currparam     (m_csiparam[m_paramidx])
#define m_disp          (consoles[curr_con].disp)
#define m_attr          (consoles[curr_con].attr)
#define m_attr_default  (consoles[curr_con].attr_default)
#define m_cursor        (consoles[curr_con].cursor)
#define m_saved         (consoles[curr_con].saved)
#define m_defaults      (consoles[curr_con].defaults)
#define m_state         (consoles[curr_con].state)

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
static void save(void);
static void restore(void);
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
static void set_vga_attr(struct vga_attr *a);
static void update_vga_attr(int pos);
static void cursor_up(int n);
static void cursor_down(int n);
static void cursor_left(int n);
static void cursor_right(int n);
static void cursor_save(void);
static void cursor_restore(void);
static void read_cursor(void);
static void write_cursor(void);
static void pos2xy(int pos, int *x, int *y);
static int xy2pos(int x, int y);



void con_init(void)
{
    vga_init();

    curr_con = 0;
    defaults(&consoles[curr_con]);
    read_cursor();
    save();
    con_write('\r');
    con_write('\n');
    m_initialized = true;
}

void con_reset(void)
{
    reset();
}

void con_save(void)
{
    save();
}

void con_restore(void)
{
    restore();
}

void con_cursor_save(void)
{
    cursor_save();
}

void con_cursor_restore(void)
{
    cursor_restore();
}

static void defaults(struct console *con)
{
    con->cols = VGA_TEXT_COLS;
    con->rows = VGA_TEXT_ROWS;
    con->framebuf = (char *) /*VGA_FRAMEBUF_COLOR*/ 0xB8000;
    con->disp.blink_on = false;
    con->attr.bg = DEFAULT_BG;
    con->attr.fg = DEFAULT_FG;
    con->attr.bright = false;
    con->attr.faint = false;
    con->attr.italic = false;
    con->attr.underline = false;
    con->attr.blink = false;
    con->attr.invert = false;
    con->cursor.x = 0;
    con->cursor.y = 0;
    con->cursor.shape = CURSOR_DEFAULT;
    con->cursor.hidden = false;
    con->defaults.attr = con->attr;
    con->defaults.cursor = con->cursor;
    con->state = S_NORM;
    con->paramidx = 0;
    memset(con->csiparam, CSIPARAM_DEFAULT, MAX_CSIPARAMS);
    for (int i = 0; i < MAX_TABSTOPS; i++) {
        m_tabstop[i] = (((i + 1) % 8) == 0);
    }
}

void con_write(char c)
{
    uint32_t flags;

    cli_save(flags);
    read_cursor();
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
        write_cursor();
    }

done:
    restore_flags(flags);
}

static void esc(char c)
{
    switch (c) {
        case '3':       /* ESC 3    disable blink */
            m_disp.blink_on = false;
            vga_disable_blink();
            break;
        case '4':       /* ESC 4    enable blink */
            m_disp.blink_on = true;
            vga_enable_blink();
            break;
        case '5':       /* ESC 5    hide cursor */
            m_cursor.hidden = true;
            vga_hide_cursor();
            break;
        case '6':       /* ESC 6    show cursor */
            m_cursor.hidden = false;
            vga_show_cursor();
            break;
        case '7':       /* ESC 7    save console */
            save();
            break;
        case '8':       /* ESC 8    restore console */
            restore();
            break;
        case 'c':       /* ESC c    reset console */
            reset();
            break;
        case 'E':       /* ESC E    newline (CRLF) */
            cr(); lf();
            break;
        case 'I':       /* ESC I    reverse line feed */
            r_lf();
            break;
        case 'M':       /* ESC M    line feed (LF) */
            lf();
            break;
        case 'T':       /* ESC T    set tab stop */
            m_tabstop[m_cursor.x] = 1;
            break;
        case 't':       /* ESC t    clear tab stop */
            m_tabstop[m_cursor.x] = 0;
            break;
        case '[':       /* ESC [    control sequence introducer */
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

#define default_param(i,x)  do { if (m_csiparam[(i)] < (x)) m_csiparam[(i)] = (x); } while (0)

    int i = 0;
    switch (c)
    {
        case 'A':       /* CSI n A      move cursor up n rows */
            default_param(0, 1);
            cursor_up(m_csiparam[0]);
            break;
        case 'B':       /* CSI n B      move cursor down n rows */
            default_param(0, 1);
            cursor_down(m_csiparam[0]);
            break;
        case 'C':       /* CSI n C      move cursor right n columns */
            default_param(0, 1);
            cursor_right(m_csiparam[0]);
            break;
        case 'D':       /* CSI n D      move cursor left n columns */
            default_param(0, 1);
            cursor_left(m_csiparam[0]);
            break;
        case 'E':       /* CSI n E      move cursor to beginning of line, n rows down */
            default_param(0, 1);
            m_cursor.x = 0;
            cursor_down(m_csiparam[0]);
            break;
        case 'F':       /* CSI n F      move cursor to beginning of line, n rows up */
            default_param(0, 1);
            m_cursor.x = 0;
            cursor_up(m_csiparam[0]);
            break;
        case 'G':       /* CSI n G      move cursor to column n */
            default_param(0, 1);
            if (m_csiparam[0] > m_cols) m_csiparam[0] = m_cols;
            m_cursor.x = m_csiparam[0] - 1;
            break;
        case 'H':       /* CSI n ; m H  move cursor row n, column m */
            default_param(0, 1);
            default_param(1, 1);
            if (m_csiparam[0] > m_rows) m_csiparam[0] = m_rows;
            if (m_csiparam[1] > m_cols) m_csiparam[1] = m_cols;
            m_cursor.y = m_csiparam[0] - 1;
            m_cursor.x = m_csiparam[1] - 1;
            break;
        case 'I':       /* CSI n I      emit n reverse-linefeeds */
            default_param(0, 1);
            for (i = 0; i < m_csiparam[0]; i++) r_lf();
            break;
        case 'M':       /* CSI n M      emit n linefeeds */
            default_param(0, 1);
            for (i = 0; i < m_csiparam[0]; i++) lf();
            break;
        case 'J':       /* CSI n J      erase in display (n = mode) */
            default_param(0, 0);
            erase(m_csiparam[0]);
            break;
        case 'K':       /* CSI n K      erase in line (n = mode) */
            default_param(0, 0);
            erase_ln(m_csiparam[0]);
            break;
        case 'S':       /* CSI n S      scroll n lines */
            default_param(0, 1);
            scroll(m_csiparam[0]);
            break;
        case 'T':       /* CSI n T      reverse scroll n lines */
            default_param(0, 1);
            scroll(-m_csiparam[0]);
            break;
        case 'm':       /* CSI n m      set graphics attribute */
            default_param(0, 0);
            while (i <= m_paramidx) csi_m(m_csiparam[i++]);
            break;
        case 's':       /* CSI s        save cursor position */
            cursor_save();
            break;
        case 'u':       /* CSI u        restore cursor position */
            cursor_restore();
            break;
        case ';':       /* (parameter separator) */
            if (++m_paramidx >= MAX_CSIPARAMS) {
                break;
            }
            return;
        default:        /* (parameter) */
            if (isdigit(c)) {
                if (m_currparam == CSIPARAM_DEFAULT) {
                    m_currparam = 0;
                }
                m_currparam *= 10;
                m_currparam += (c - '0');
                return;
            }
            break;
    }

    m_state = S_NORM;
}

static void csi_m(char p)
{
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
        case 3:
            m_attr.italic = true;
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
        case 23:
            m_attr.italic = false;
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
            /* TODO: programmable defaults */
            if (p == 38 || p == 39) m_attr.fg = m_defaults.attr.fg;
            if (p == 48 || p == 49) m_attr.bg = m_defaults.attr.bg;
            break;
    }
}

static void save(void)
{
    m_saved.disp = m_disp;
    m_saved.attr = m_attr;
    cursor_save();
    memcpy(&m_saved.tabstop, &m_tabstop, MAX_TABSTOPS);
}

static void restore(void)
{
    m_disp = m_saved.disp;
    m_attr = m_saved.attr;
    cursor_restore();
    memcpy(&m_tabstop, &m_saved.tabstop, MAX_TABSTOPS);
}

static void reset(void)
{
    defaults(&consoles[curr_con]);
    write_cursor();
    erase(0);
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
    struct vga_cell *start = NULL;
    int count = 0;
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
        // default:
        //     // TODO: assert!
    }

    for (int i = 0; i < count; i++) {
        start[i].ch = BLANK_CHAR;
        set_vga_attr(&start[i].attr);
    }
}

static void erase_ln(int mode)
{
    struct vga_cell *start = NULL;
    int count = 0;
    int pos = xy2pos(m_cursor.x, m_cursor.y);
    int area = m_cols;

    switch (mode) {
        case 0:     /* erase line from cursor down */
            start = &m_framebuf[pos];
            count = area - (pos % m_cols);
            break;
        case 1:     /* erase line from cursor up /*/
            start = &m_framebuf[xy2pos(0, m_cursor.y)];
            count = (pos % m_cols) + 1;
            break;
        case 2:     /* erase entire line */
            start = &m_framebuf[xy2pos(0, m_cursor.y)];
            count = area;
    }

    for (int i = 0; i < count; i++) {
        start[i].ch = BLANK_CHAR;
        set_vga_attr(&start[i].attr);
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
    vga_set_cursor_shape(m_cursor.shape & 0xFF, m_cursor.shape >> 8);
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

static void update_vga_attr(int pos)
{
    set_vga_attr(&m_framebuf[pos].attr);
}


/**
 * Exchanges two values.
 */
#define swap(a,b)           \
do {                        \
    (a) ^= (b);             \
    (b) ^= (a);             \
    (a) ^= (b);             \
} while(0)

static void set_vga_attr(struct vga_attr *a)
{
    a->bg = m_attr.bg;
    a->fg = m_attr.fg;

    if (m_attr.bright) {
        a->bright = 1;
    }
    if (m_attr.faint) {
        a->fg = VGA_BLK;    /* simulate with dark gray */
        a->bright = 1;
    }
    if (m_attr.italic) {
        a->fg = VGA_GRN;    /* simulate with green */
        a->bright = 0;
    }
    if (m_attr.underline) {
        a->fg = VGA_CYN;    /* simulate with cyan */
        a->bright = 0;
    }
    if (m_attr.blink) {
        a->blink = 1;
    }
    if (m_attr.invert) {
        swap(a->color_bg, a->color_fg);
    }
}
