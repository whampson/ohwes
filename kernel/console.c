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
#include <paging.h>
#include <x86.h>

#define DEFAULT_FB              __phys_to_virt(0xB8000)
#define PAGES_PER_CONSOLE_FB    2   // 8192 chars (enough for 80x50 in text mode)

// TODO: set via ioctl
#define BELL_FREQ               750     // Hz
#define BELL_TIME               50      // ms
#define ALERT_FREQ              1675
#define ALERT_TIME              50

// global vars
struct vga _vga = { };
struct vga *g_vga = &_vga;
struct console g_consoles[NUM_CONSOLES];

#define has_iflag(cons,iflag)   (has_flag((cons)->termios.c_iflag, iflag))
#define has_oflag(cons,oflag)   (has_flag((cons)->termios.c_oflag, oflag))
#define has_lflag(cons,lflag)   (has_flag((cons)->termios.c_lflag, lflag))

enum console_state {
    S_NORM,
    S_ESC,
    S_CSI
};

// console state
static void defaults(struct console *cons);
static void reset(struct console *cons);                // ESC c
static void save_console(struct console *cons);         // ESC 7
static void restore_console(struct console *cons);      // ESC 8
static void cursor_save(struct console *cons);          // ESC [s
static void cursor_restore(struct console *cons);       // ESC [u

// character handling
static void write_char(struct console *cons, char c);
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
static void set_vga_blink_enable(struct console *cons); // ESC 3 / ESC 4
static void set_vga_cursor_enable(struct console *cons);// ESC 5 / ESC 6
static void set_vga_cursor_pos(struct console *cons);   // ESC [ <n>;<m>H
static void set_vga_cursor_shape(struct console *cons);
static void set_vga_char(struct console *cons, uint16_t pos, char c);
static void set_vga_attr(struct console *cons, uint16_t pos, struct _char_attr attr);

// screen positioning
static void pos2xy(uint16_t ncols, uint16_t pos, uint16_t *x, uint16_t *y);
static uint16_t xy2pos(uint16_t ncols, uint16_t x, uint16_t y);

// ----------------------------------------------------------------------------
// initialization

extern void init_kb(const struct boot_info *info);

void init_console(const struct boot_info *info)
{
    zeromem(g_vga, sizeof(struct vga));
    zeromem(g_consoles, sizeof(struct console) * NUM_CONSOLES);

    // get VGA info from boot info
    g_vga->active_console = 1;
    g_vga->rows = info->vga_rows;
    g_vga->cols = info->vga_cols;

    // select frame buffer based on config
    switch (VGA_FB_SELECT) {
        case VGA_FB_128K:
            g_vga->fb = (void *) __phys_to_virt(0xA0000);
            g_vga->fb_size_pages = 32;
            break;
        case VGA_FB_64K:
            g_vga->fb = (void *) __phys_to_virt(0xA0000);
            g_vga->fb_size_pages = 16;
            break;
        case VGA_FB_32K_LO:
            g_vga->fb = (void *) __phys_to_virt(0xB0000);
            g_vga->fb_size_pages = 8;
            break;
        case VGA_FB_32K_HI:
            g_vga->fb = (void *) __phys_to_virt(0xB8000);
            g_vga->fb_size_pages = 8;
            break;
        default:
            ((uint16_t *) DEFAULT_FB)[0] = 0x4700|'X';
            for (;;);
    }

    // make sure we have enough memory for the configured number of consoles
    if (g_vga->fb_size_pages - PAGES_PER_CONSOLE_FB < NUM_CONSOLES * PAGES_PER_CONSOLE_FB) {
        ((uint16_t *) DEFAULT_FB)[0] = 0x4700|'!';
        for(;;);
    }

    // copy the existing frame buffer
    memcpy(g_vga->fb, (void *) DEFAULT_FB, PAGES_PER_CONSOLE_FB);

    // program the VGA frame buffer select
    {
        uint8_t ram_select = VGA_FB_SELECT;
        uint8_t grfx_misc = vga_grfx_read(VGA_GRFX_REG_MISC);
        grfx_misc = (grfx_misc & 0xF3) | (ram_select << 2);
        vga_grfx_write(VGA_GRFX_REG_MISC, grfx_misc);
    }

    // read cursor attributes leftover from BIOS
    uint16_t cursor_x, cursor_y;
    {
        uint8_t lo, hi;
        lo = vga_crtc_read(VGA_CRTC_REG_CL_LO);
        hi = vga_crtc_read(VGA_CRTC_REG_CL_HI);
        pos2xy(g_vga->cols, (hi << 8) | lo, &cursor_x, &cursor_y);
        lo = vga_crtc_read(VGA_CRTC_REG_CSS) & VGA_CRTC_FLD_CSS_CSS_MASK;
        hi = vga_crtc_read(VGA_CRTC_REG_CSE) & VGA_CRTC_FLD_CSE_CSE_MASK;
        g_vga->orig_cursor_shape = (hi << 8) | lo;
    }

    // initialize virtual consoles
    for (int i = 1; i <= NUM_CONSOLES; i++) {
        struct console *cons = &g_consoles[i - 1];
        cons->framebuf = g_vga->fb;
        cons->framebuf += (i * PAGES_PER_CONSOLE_FB) << PAGE_SHIFT;
        cons->number = i;
        defaults(cons);
    }

    struct console *cons = get_console(1);
    cons->cursor.x = cursor_x;
    cons->cursor.y = cursor_y;

    // create a restore point
    save_console(cons);

    // do a proper 'switch' to the initial console
    int ret = switch_console(1);
    if (ret != 0) {
        panic("failed to initialize console!");
    }

    // safe to print now
    kprint("\r\n\e4\e6");
    kprint("\e[0;1m" OS_NAME " " OS_VERSION ", build: " OS_BUILDDATE " ]]\e[0m\n");

    // get the keyboard working
    init_kb(info);
}

// ----------------------------------------------------------------------------
// public functions

int switch_console(int num)
{
    if (num <= 0 || num > NUM_CONSOLES) {
        return -EINVAL;
    }

    struct console *curr = current_console();
    struct console *next = get_console(num);

    uint32_t pgdir_addr = 0;
    read_cr3(pgdir_addr);
    pgdir_addr = __phys_to_virt(pgdir_addr);

    // enable kernel identity mapping so we can operate on page tables
    pde_t *ident_pde = (pde_t *) pgdir_addr;
    *ident_pde = __mkpde(KERNEL_PGTBL, _PAGE_RW);

    // identity map old frame buffer, so it will write to back buffer
    for (int i = 0; i < PAGES_PER_CONSOLE_FB; i++) {
        uint32_t fb_page = (uint32_t) curr->framebuf + (i << PAGE_SHIFT);
        pte_t *pte = pte_offset((pde_t *) pgdir_addr, fb_page);
        *pte = __mkpte(__virt_to_phys(fb_page), _PAGE_RW);
    }
    flush_tlb();

    // swap buffers
    memcpy(curr->framebuf, g_vga->fb, PAGE_SIZE * PAGES_PER_CONSOLE_FB);
    memcpy(g_vga->fb, next->framebuf, PAGE_SIZE * PAGES_PER_CONSOLE_FB);
    curr = next;

    // map new frame buffer to VGA
    for (int i = 0; i < PAGES_PER_CONSOLE_FB; i++) {
        uint32_t fb_page = (uint32_t) curr->framebuf + (i << PAGE_SHIFT);
        uint32_t vga_page = (intptr_t) (g_vga->fb + (i << PAGE_SHIFT));
        pte_t *pte = pte_offset((pde_t *) pgdir_addr, fb_page);
        *pte = __mkpte(__virt_to_phys(vga_page), _PAGE_RW);
    }

    // clear kernel identity mapping and flush TLB
    pde_clear(ident_pde);
    flush_tlb();

    // update VGA state
    g_vga->active_console = curr->number;
    set_vga_blink_enable(curr);
    set_vga_cursor_enable(curr);
    set_vga_cursor_shape(curr);
    set_vga_cursor_pos(curr);

    // a console is fully initialized once we've switch to it at least once :-)
    curr->initialized = true;
    return 0;
}

struct console * current_console(void)
{
    return get_console(0);
}

struct console * get_console(int num)
{
    if (num < 0 || num > NUM_CONSOLES) {
        return NULL;
    }

    if (num == 0) {
        num = g_vga->active_console;
    }
    panic_assert(num > 0 && (num - 1) < NUM_CONSOLES);

    struct console *cons = &g_consoles[num - 1];
    panic_assert(cons->number == num);

    return cons;
}

int console_read(struct console *cons, char *buf, size_t count)
{
    int nread;
    uint32_t flags;

    if (!cons || !buf) {
        return -EINVAL;
    }

    // TODO: make sure this comes from the correct console for the
    // calling process!!

    nread = 0;
    while (count--) {
        spin(char_queue_empty(&cons->inputq));    // block until a character appears
        // TODO: allow nonblocking input

        cli_save(flags);
        *buf++ = char_queue_get(&cons->inputq);
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
        write_char(cons, buf[i]);
#if E9_HACK
        if (cons == get_console(1)) {
            outb(0xE9, buf[i]);
        }
#endif
    }

    return count;
}

int console_recv(struct console *cons, char c)  // called from ps2kb.c
{
    int count;

    if (!cons) {
        return -EINVAL;
    }

    if (char_queue_full(&cons->inputq)) {
        beep(ALERT_FREQ, ALERT_TIME);
        return 0;
    }

    // input processing
    if (c == '\r') {
        if (has_iflag(cons, IGNCR)) {
            return 0;
        }
        if (has_iflag(cons, ICRNL)) {
            c = '\n';
        }
    }
    else if (c == '\n' && has_iflag(cons, INLCR)) {
        c = '\r';
    }

    // TODO: leave space for \n, etc.
    char_queue_put(&cons->inputq, (char) c);
    count = 1;

    // echoing
    if (has_lflag(cons, ECHO)) {
        if (has_lflag(cons, ECHOCTL) && iscntrl(c)) {
            if (c != '\t') {
                write_char(cons, '^');
                count++;
                if (c == 0x7F) {
                    c -= 0x40;
                }
                else {
                    c += 0x40;
                }
            }
        }
        write_char(cons, c);
    }

    return count;
}

// ----------------------------------------------------------------------------
// private functions

void defaults(struct console *cons)
{
    cons->state = S_NORM;
    cons->cols = g_vga->cols;
    cons->rows = g_vga->rows;
    char_queue_init(&cons->inputq, cons->_ibuf, INPUT_BUFFER_SIZE);
    for (int i = 0; i < MAX_TABSTOPS; i++) {
        cons->tabstops[i] = (((i + 1) % TABSTOP_WIDTH) == 0);
    }
    memset(cons->csiparam, -1, MAX_CSIPARAMS);
    cons->paramidx = 0;
    cons->blink_on = false;
    cons->need_wrap = false;
    cons->termios.c_iflag = DEFAULT_IFLAG;
    cons->termios.c_oflag = DEFAULT_OFLAG;
    cons->termios.c_lflag = DEFAULT_LFLAG;
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
    erase(cons, 2);
    save_console(cons);
}

static void reset(struct console *cons)
{
    defaults(cons);
    if (cons == current_console()) {
        set_vga_blink_enable(cons);
        set_vga_cursor_enable(cons);
        set_vga_cursor_shape(cons);
        set_vga_cursor_pos(cons);
    }
}

static void save_console(struct console *cons)
{
    memcpy(cons->saved_state.tabstops, cons->tabstops, MAX_TABSTOPS);
    cons->saved_state.blink_on = cons->blink_on;
    cons->saved_state.attr = cons->attr;
    cursor_save(cons);
}

static void restore_console(struct console *cons)
{
    memcpy(cons->tabstops, cons->saved_state.tabstops, MAX_TABSTOPS);
    cons->blink_on = cons->saved_state.blink_on;
    cons->attr = cons->saved_state.attr;
    cursor_restore(cons);
}

static void cursor_save(struct console *cons)
{
    cons->saved_state.cursor = cons->cursor;
}

static void cursor_restore(struct console *cons)
{
    cons->cursor = cons->saved_state.cursor;
}

static void write_char(struct console *cons, char c)
{
    bool update_char = false;
    bool update_attr = false;
    bool update_cursor_pos = true;
    uint16_t char_pos;

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
            beep(BELL_FREQ, BELL_TIME);
            break;
        case '\b':      // ^H - BS - backspace
            backspace(cons);
            break;
        case '\t':      // ^I - HT - horizontal tab
            tab(cons);
            break;
        case '\n':      // ^J - LF - line feed
            if (has_oflag(cons, OPOST) && has_oflag(cons, ONLCR)) {
                carriage_return(cons);
            }
            __fallthrough;
        case '\v':      // ^K - VT - vertical tab
        case '\f':      // ^L - FF - form feed
            line_feed(cons);
            break;
        case '\r':      // ^M - CR -  carriage return
            if (has_oflag(cons, OPOST) && has_oflag(cons, OCRNL)) {
                line_feed(cons);
            }
            else {
                carriage_return(cons);
            }
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
        set_vga_char(cons, char_pos, c);
    }
    if (update_attr) {
        if (cons->attr.bright && cons->attr.faint) {
            cons->attr.bright = false;      // faint overrides bright
        }
        set_vga_attr(cons, char_pos, cons->attr);
    }
    if (update_cursor_pos && cons == current_console()) {
        set_vga_cursor_pos(cons);
    }

done:
    return;
}

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
            memset(cons->csiparam, -1, MAX_CSIPARAMS);
            cons->paramidx = 0;
            cons->state = S_CSI;
            return;

        //
        // "Custom" console-related sequences
        //
        case '3':       // ESC 3    disable blink
            cons->blink_on = false;
            if (cons == current_console()) {
                set_vga_blink_enable(cons);
            }
            break;
        case '4':       // ESC 4    enable blink
            cons->blink_on = true;
            if (cons == current_console()) {
                set_vga_blink_enable(cons);
            }
            break;
        case '5':       // ESC 5    hide cursor
            cons->cursor.hidden = true;
            if (cons == current_console()) {
                set_vga_cursor_enable(cons);
            }
            break;
        case '6':       // ESC 6    show cursor
            cons->cursor.hidden = false;
            if (cons == current_console()) {
                set_vga_cursor_enable(cons);
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
            if (cons->paramidx >= MAX_CSIPARAMS) {
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
        set_vga_char(cons, pos, ' ');
        set_vga_attr(cons, pos, cons->attr);
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
        set_vga_char(cons, start + i, ' ');
        set_vga_attr(cons, start + i, cons->attr);
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
        set_vga_char(cons, start + i, ' ');
        set_vga_attr(cons, start + i, cons->attr);
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

static void pos2xy(uint16_t ncols, uint16_t pos, uint16_t *x, uint16_t *y)
{
    *x = pos % ncols;
    *y = pos / ncols;
}

static uint16_t xy2pos(uint16_t ncols, uint16_t x, uint16_t y)
{
    return y * ncols + x;
}

// ----------------------------------------------------------------------------

static void set_vga_blink_enable(struct console *cons)
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

static void set_vga_cursor_enable(struct console *cons)
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

static void set_vga_cursor_pos(struct console *cons)
{
    uint32_t flags;
    uint16_t pos;

    pos = xy2pos(cons->cols, cons->cursor.x, cons->cursor.y);

    cli_save(flags);
    vga_crtc_write(VGA_CRTC_REG_CL_HI, pos >> 8);
    vga_crtc_write(VGA_CRTC_REG_CL_LO, pos & 0xFF);
    restore_flags(flags);
}

static void set_vga_cursor_shape(struct console *cons)
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

static void set_vga_char(struct console *cons, uint16_t pos, char c)
{
    ((struct vga_cell *) cons->framebuf)[pos].ch = c;
}

static void set_vga_attr(struct console *cons, uint16_t pos, struct _char_attr attr)
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
