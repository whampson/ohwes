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
 * =============================================================================
 */

#ifndef __TERMINAL_H
#define __TERMINAL_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <ring.h>
#include <kernel/tty.h>
#include <kernel/vga.h>

// TODO: set via ioctl
#define BELL_FREQ               750     // Hz
#define BELL_TIME               100     // ms

#define MAX_CSIPARAM            16      // ESC[p;q;r;s;...,n param count
#define MAX_TABSTOP             80      // maximum number of tabstops allowed
#define TABSTOP_WIDTH           8       // TODO: make configurable

struct terminal {
    int number;                         // virtual terminal number
    int state;                          // current control state
    bool initialized;                   // terminal has been switched to once
    uint32_t printing;                  // terminal is currently printing (atomic)

    struct tty *tty;
    int refcount;

    uint16_t cols, rows;                // screen dimensions
    void *framebuf;                     // frame buffer, always valid (i.e. when visible or hidden)
    uintptr_t backbuf;                  // physical address of terminal's back buffer
    uint16_t origin;                    // scan line starting word offset within frame buffer

    char tabstops[MAX_TABSTOP];         // tab stops

    int csiparam[MAX_CSIPARAM];         // control sequence parameters
    int paramidx;                       // control sequence parameter index

    bool blink_on;                      // character blinking enabled
    bool line_wrap_pending;             // a line wrap is pending

    struct keyboard_state kb_state;     // current keyboard state

    struct _char_attr {                 // character attributes
        union {
            struct {
                uint32_t fg        : 8; // foreground color
                uint32_t bg        : 8; // background color
                uint32_t bold      : 1; // bright/high-intensity foreground
                uint32_t faint     : 1; // dim foreground (simulated with dark gray)
                uint32_t italic    : 1; // italicize (simulated with green)
                uint32_t underline : 1; // underline (simulated with cyan)
                uint32_t strike    : 1; // strikethrough (simulated with magenta)
                uint32_t conceal   : 1; // hide text (foreground set to background)
                uint32_t blink     : 1; // make character blink, or control background intensity
                uint32_t invert    : 1; // swap background and foreground colors
            };
            uint32_t _value;
        };
    } attr;
    static_assert(sizeof(struct _char_attr) == 4, "_char_attr too large!");

    struct _cursor {                    // cursor parameters
        union {
            struct {
                uint64_t shape : 16;    //   start/end scan line
                uint64_t x     : 12;    //   x position (4096 max)
                uint64_t y     : 12;    //   y position (4096 max)
                uint64_t hidden : 1;    //   visibility
            };
            uint64_t _value;
        };
    } cursor;
    static_assert(sizeof(struct _cursor) == 8, "_cursor too large!");

    struct _csi_defaults {              // ESC [0m
        struct _char_attr attr;
        struct _cursor cursor;
    } csi_defaults;

    struct terminal_save_state {        // ESC 7 / ESC 8
        bool blink_on;
        char tabstops[MAX_TABSTOP];
        uint32_t attr;
        uint64_t cursor;                // ESC [s / ESC [u
    } saved_state;
};

// get virtual terminal; 0 returns current terminal
struct terminal * get_terminal(int num);

// switch to a virtual terminal
int switch_terminal(int num);

// has the keyboard driver been configured
// and is the keyboard currently usable?
bool kb_avail(void);
bool kb_sysrq_enabled(void);        // will SysRq functions work?
void kb_enable_sysrq(bool enable);  // enable/disable SysRq key
bool kb_int3_enabled(void);         // will Ctrl+Alt+F3 trigger a debug break?
void kb_enable_int3(bool enable);   // enable/disable debug break via keyboard

// write directly to terminal, bypassing TTY
void __terminal_putc(struct terminal *term, char c);


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
