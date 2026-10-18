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
 *         File: include/kernel/terminal.h
 *      Created: October 10, 2023
 *       Author: Wes Hampson
 *
 * Virtual Terminal / VGA Terminal / "The Terminal"
 *
 * 'vgaterm' or 'terminal' refers to the VT-100'esque implementation of the
 *    video terminal using the VGA text mode features.
 * 'vt' is short for "virtual terminal", referring to system that allows for
 *    multiple virtualized VGA terminals on a single physical monitor/keyboard.
 * 'tty' is the standard character stream I/O interface to the kernel, short
 *    for "TeleType" for archaic yet fundamental reasons
 * =============================================================================
 */

#ifndef __TERMINAL_H
#define __TERMINAL_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <kernel/tty.h>
#include <kernel/vga.h>

#define MAX_CSIPARAM            16      // ESC[p;q;r;s;... param count
#define MAX_TABSTOP             256     // max number of tabstops per line
#define TABSTOP_WIDTH           8       // TODO: make configurable

//
// 'Set Graphics Rendition' Color Codes
//
enum sgr_color {
    _SGR_BRIGHT_BIT    = 0b1000,

    SGR_BLACK          = 0,
    SGR_RED            = 1,
    SGR_GREEN          = 2,
    SGR_YELLOW         = 3,
    SGR_BLUE           = 4,
    SGR_MAGENTA        = 5,
    SGR_CYAN           = 6,
    SGR_WHITE          = 7,
    SGR_GRAY           = (_SGR_BRIGHT_BIT | SGR_BLACK),
    SGR_BRIGHT_RED     = (_SGR_BRIGHT_BIT | SGR_RED),
    SGR_BRIGHT_GREEN   = (_SGR_BRIGHT_BIT | SGR_GREEN),
    SGR_BRIGHT_YELLOW  = (_SGR_BRIGHT_BIT | SGR_YELLOW),
    SGR_BRIGHT_BLUE    = (_SGR_BRIGHT_BIT | SGR_BLUE),
    SGR_BRIGHT_MAGENTA = (_SGR_BRIGHT_BIT | SGR_MAGENTA),
    SGR_BRIGHT_CYAN    = (_SGR_BRIGHT_BIT | SGR_CYAN),
    SGR_BRIGHT_WHITE   = (_SGR_BRIGHT_BIT | SGR_WHITE),
};

enum cursor_style {
    CURSOR_BLOCK     = 1,
    CURSOR_UNDERLINE = 3,
    // 2 and 4 are non-blinking variants, which are not yet supported
};

//
// VGA Terminal
//
struct terminal {
    //
    // immutable fields; cannot be controlled by user terminal input
    //

    int number;                         // virtual terminal number
    bool initialized;                   // terminal has been switched to once
    uint32_t printing;                  // terminal is currently printing (atomic)

    struct tty *tty;                    // attached TTY
    int refcount;                       // open TTY references

    uint16_t cols, rows;                // screen dimensions
    void *backbuf;                      // virtual frame buffer, always valid (i.e. when visible or hidden)
    uint16_t origin;                    // scan line starting word offset within frame buffer

    struct keyboard_state kb_state;     // current keyboard state

    //
    // mutable fields; controllable by user input to terminal
    //

    char tabstops[MAX_TABSTOP>>3];      // tab stops (bitmap)

    int seq_state;                      // current control sequence state
    int csiparam[MAX_CSIPARAM];         // control sequence parameters
    int paramidx;                       // control sequence parameter index

    struct _display_state {
        union {
            struct {
                uint32_t switch_to     : 8; // switch to selected terminal
                uint32_t blanked       : 1; // display contents are temporarily hidden
                uint32_t blink_on      : 1; // character blinking enabled
                uint32_t reverse_video : 1; // all foreground and background colors swapped
                uint32_t pending_wrap  : 1; // a line wrap is pending
                uint32_t autowrap      : 1; // auto-wrap enabled (ESC[?7h/l)
            };
            uint32_t _value;
        };
    } disp;

    struct _sgr_attr {
        union {
            struct {
                uint32_t fg     : 4;    // foreground color
                uint32_t bg     : 4;    // background color
                uint32_t bold   : 1;    // bright/high-intensity foreground
                uint32_t faint  : 1;    // dim foreground (simulated with dark gray)
                uint32_t italic : 1;    // italicize (simulated with green)
                uint32_t undline: 1;    // underline (simulated with cyan)
                uint32_t strike : 1;    // strikethrough (simulated with magenta)
                uint32_t conceal: 1;    // hide text (foreground set to background)
                uint32_t blink  : 1;    // make character blink, or control background intensity
                uint32_t invert : 1;    // swap background and foreground colors
            };
            uint32_t _value;
        };
    } sgr;                              // current character attributes

    struct _cursor {
        union {
            struct {
                union {
                    struct {
                        uint32_t top    : 8;    // start scan line within row
                        uint32_t bottom : 8;    // end scan line within row
                    };
                    uint32_t shape : 16;    // packed scan start/end line
                };
                uint32_t x      : 8;    // x coord
                uint32_t y      : 8;    // y coord
                uint32_t hidden : 1;    // cursor invisible
            };
            uint32_t _value;
        };
    } cursor;                           // current cursor parameters

    struct _term_defaults {
        struct {
            uint8_t fg      : 4;        // ESC[=0;0;<n>~  - default foreground color
            uint8_t bg      : 4;        // ESC[=0;1;<n>~  - default background color
            uint8_t faint   : 4;        // ESC[=0;2;<n>~  - default 'faint' color
            uint8_t italic  : 4;        // ESC[=0;3;<n>~  - default 'italic' color
            uint8_t undline : 4;        // ESC[=0;4;<n>~  - default 'underline' color
            uint8_t strike  : 4;        // ESC[=0;9;<n>~  - default 'strikethrough' color
        } sgr;
        struct {
            uint16_t hz;                // ESC[=7;<n>~    - default BEL frequency
            uint16_t ms;                // ESC[=7;<n>~    - default BEL duration
        } bell;
        enum cursor_style cursor_style; // ESC[<n>q       - cursor style
    } defaults;

    struct _term_save_state {
        char tabstops[MAX_TABSTOP>>3];
        struct _display_state disp;
        struct _sgr_attr sgr;
        struct _cursor cursor;          // saved cursor pos (ESC [s / ESC [u)
    } saved_state;                      // saved terminal state (ESC 7 / ESC 8)
};

// get virtual terminal; 0 returns current terminal
struct terminal * get_terminal(int num);

// switch to a virtual terminal
int switch_terminal(int num);

// write directly to terminal, bypassing TTY
void __terminal_putc(struct terminal *term, char c);

// has the keyboard driver been configured
// and is the keyboard currently usable?
bool kb_avail(void);
bool kb_sysrq_enabled(void);        // will SysRq functions work?
void kb_enable_sysrq(bool enable);  // enable/disable SysRq key
bool kb_int3_enabled(void);         // will Ctrl+Alt+F3 trigger a debug break?
void kb_enable_int3(bool enable);   // enable/disable debug break via keyboard

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

#endif // __TERMINAL_H
