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

#include <config.h>
#include <stdbool.h>
#include <stdint.h>
#include <queue.h>

#define MAX_CSIPARAMS       8
#define MAX_TABSTOPS        /*VGA_COLS*/ 80     // TODO: make this not depend on console width
#define TABSTOP_WIDTH       8

#define INPUT_BUFFER_SIZE   256

enum vga_console_state {
    S_NORM,
    S_ESC,
    S_CSI
};

enum vga_console_color {
    VGA_CONSOLE_BLACK,
    VGA_CONSOLE_RED,
    VGA_CONSOLE_GREEN,
    VGA_CONSOLE_YELLOW,
    VGA_CONSOLE_BLUE,
    VGA_CONSOLE_MAGENTA,
    VGA_CONSOLE_CYAN,
    VGA_CONSOLE_WHITE
};

struct vga_console
{
    bool initialized;                   // console initalized?
    int state;                          // current control state

    uint16_t cols, rows;                // screen dimensions
    void *framebuf;                     // frame buffer

    struct char_queue inputq;           // input queue
    char input_buf[INPUT_BUFFER_SIZE];  // raw input ring buffer

    char tabstops[MAX_TABSTOPS];        // tab stops

    char csiparam[MAX_CSIPARAMS];       // control sequence parameters
    int paramidx;                       // control sequence parameter index

    bool blink_on;                      // character blinking enabled

    struct console_char_attr {          // character attributes
        uint8_t bg, fg;                 //   background, foreground colors
        bool bright;                    //   use bright foreground
        bool faint;                     //   use dim foreground
        bool italic;                    //   italicize (simulated with color
        bool underline;                 //   underline (simulated with color)
        bool blink;                     //   blink character (if enabled)
        bool invert;                    //   swap background and foreground colors
    } attr;

    struct console_cursor {             // cursor parameters
        uint16_t x, y;                  //   position
        int shape;                      //   shape
        bool hidden;                    //   visibility
    } cursor;

    struct console_csi_defaults {       // CSI defaults (ESC [0m)
        struct console_char_attr attr;
        struct console_cursor cursor;
    } csi_defaults;

    struct console_save_state {         // saved parameters
        bool blink_on;
        char tabstops[MAX_TABSTOPS];
        struct console_char_attr attr;
        struct console_cursor cursor;
    } saved_state;
};

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
