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
 *         File: include/console.h
 *      Created: October 10, 2023
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __CONSOLE_H
#define __CONSOLE_H

#include <assert.h>
#include <config.h>
#include <stdbool.h>
#include <stdint.h>
#include <queue.h>

#define MAX_CSIPARAMS       16
#define MAX_TABSTOPS        80      // TODO: make this not depend on console width
#define TABSTOP_WIDTH       8       // TOOD: make configurable

#define INPUT_BUFFER_SIZE   128

#define DEFAULT_IFLAG       0
#define DEFAULT_OFLAG       (OPOST|ONLCR)
#define DEFAULT_LFLAG       ECHO

enum console_color {
    CONSOLE_BLACK,
    CONSOLE_RED,
    CONSOLE_GREEN,
    CONSOLE_YELLOW,
    CONSOLE_BLUE,
    CONSOLE_MAGENTA,
    CONSOLE_CYAN,
    CONSOLE_WHITE
};

enum iflag {
    INLCR = 1 << 0,     // map NL to CR
    IGNCR = 1 << 1,     // ignore carriage return
    ICRNL = 1 << 2,     // map CR to NL (unless IGNCR is set)
};

enum oflag {
    OPOST = 1 << 0,     // enable post processing
    ONLCR = 1 << 1,     // convert NL to CRNL
    OCRNL = 1 << 2,     // map CR to NL
};

enum lflag {
    ECHO = 1 << 0,      // echo input characters
    ECHOCTL = 1 << 1,   // if ECHO is also set, echo control characters as ^X
};

struct termios {
    uint32_t c_iflag;
    uint32_t c_oflag;
    uint32_t c_lflag;
};

struct vga {
    uint32_t active_console;
    uint32_t rows, cols;
    uint32_t fb_size_pages;
    uint16_t orig_cursor_shape;
    void *fb;
};

struct console
{
    int number;                         // console I/O line number
    int state;                          // current control state
    bool initialized;                   // console can be used

    uint16_t cols, rows;                // screen dimensions
    void *framebuf;                     // frame buffer

    struct char_queue inputq;           // input queue
    char _ibuf[INPUT_BUFFER_SIZE];      // raw input ring buffer

    char tabstops[MAX_TABSTOPS];        // tab stops    // TODO: make indexing independent of console width

    int csiparam[MAX_CSIPARAMS];       // control sequence parameters
    int paramidx;                       // control sequence parameter index

    bool blink_on;                      // character blinking enabled
    bool need_wrap;                     // wrap output to next line on next character

    struct termios termios;             // terminal input/output behavior

    struct _char_attr {                 // character attributes
        uint8_t bg, fg;                 //   background, foreground colors
        uint8_t bright    : 1;          //   use bright foreground
        uint8_t faint     : 1;          //   use dim foreground
        uint8_t italic    : 1;          //   italicize (simulated with color
        uint8_t underline : 1;          //   underline (simulated with color)
        uint8_t blink     : 1;          //   blink character (if enabled)
        uint8_t invert    : 1;          //   swap background and foreground colors
    } attr;
    static_assert(sizeof(struct _char_attr) <= 4, "_char_attr too large!");

    struct _cursor {                    // cursor parameters
        uint16_t x, y;                  //   position
        int shape;                      //   shape
        bool hidden;                    //   visibility
    } cursor;

    struct _csi_defaults {              // CSI defaults (ESC [0m)
        struct _char_attr attr;
        struct _cursor cursor;
    } csi_defaults;

    struct _save_state {                // saved parameters
        bool blink_on;
        char tabstops[MAX_TABSTOPS];
        struct _char_attr attr;
        struct _cursor cursor;
    } saved_state;
};

struct console * current_console(void);
struct console * get_console(int num);      // indexed at 1, 0 = current console

int switch_console(int num);

int console_read(struct console *cons, char *buf, size_t count);
int console_write(struct console *cons, const char *buf, size_t count);

/**
 * ASCII Control Characters
 */
enum ascii_cntl {
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

#endif /* __CONSOLE_H */
