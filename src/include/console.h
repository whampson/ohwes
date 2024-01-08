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
 *    File: include/ohwes/console.h                                           *
 * Created: December 14, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#ifndef __CONSOLE_H
#define __CONSOLE_H

#include <stdbool.h>
#include <stdint.h>

#define VGA_COLS            80
#define VGA_ROWS            25
#define VGA_FRAMEBUF        0xB8000

#define ERASE_DOWN          0
#define ERASE_UP            1
#define ERASE_ALL           2

#define MAX_CSIPARAMS       8
#define MAX_TABSTOPS        VGA_COLS
#define TABSTOP_WIDTH       8

struct console
{
    bool initialized;               // console initalized?
    int cols, rows;                 // screen dimensions
    void *framebuf;                 // frame buffer
    char tabstop[MAX_TABSTOPS];     // tab stops
    char csiparam[MAX_CSIPARAMS];   // control sequence parameters
    int paramidx;                   // control sequence parameter index
    struct disp_attr {              // display attributes
        bool blink_on;              //   character blinking enabled
    } disp;
    struct char_attr {              // character attributes
        int bg, fg;                 //   background, foreground colors
        bool bright;                //   use bright foreground
        bool faint;                 //   use dim foreground
        bool italic;                //   italicize (simulated with color
        bool underline;             //   underline (simulated with color)
        bool blink;                 //   blink character (if enabled)
        bool invert;                //   swap background and foreground colors
    } attr;
    struct cursor {                 // cursor parameters
        int x, y;                   //   position
        int shape;                  //   shape
        bool hidden;                //   visibility
    } cursor;
    struct save_state {             // saved parameters
        struct disp_attr disp;
        struct char_attr attr;
        struct cursor cursor;
        char tabstop[MAX_TABSTOPS];
    } saved;
    struct default_state {          // default parameters
        struct char_attr attr;
        struct cursor cursor;
    } defaults;
    enum console_state {            // console control state
        S_NORM,                     //   normal
        S_ESC,                      //   escape sequence (ESC)
        S_CSI                       //   control sequence (ESC[)
    } state;
};

void console_write(char c);
void console_reset(void);
void console_save(void);
void console_restore(void);
void console_save_cursor(void);
void console_restore_cursor(void);

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
