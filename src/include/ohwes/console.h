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
#include <drivers/vga.h>

#define NUM_CONSOLES        8
#define MAX_CSIPARAMS       8
#define MAX_TABSTOPS        (VGA_TEXT_COLS)

/**
 * System Console
 */
struct console
{
    bool initialized;               /* console initalized? */
    int cols, rows;                 /* screen dimensions */
    char *framebuf;                 /* frame buffer */
    char tabstop[MAX_TABSTOPS];     /* tab stops */
    char csiparam[MAX_CSIPARAMS];   /* control sequence parameters */
    int paramidx;                   /* control sequence parameter index */
    struct disp_attr {              /* display attributes */
        bool blink_on;              /*   character blinking enabled */
    } disp;
    struct char_attr {              /* character attributes */
        int bg, fg;                 /*   background, foreground colors */
        bool bright;                /*   use bright foreground */
        bool faint;                 /*   use dim foreground */
        bool italic;                /*   italicize (simulated with color)*/
        bool underline;             /*   underline (simulated with color) */
        bool blink;                 /*   blink character (if enabled) */
        bool invert;                /*   swap background and foreground colors */
    } attr;
    struct cursor {                 /* cursor parameters */
        int x, y;                   /*   position */
        int shape;                  /*   shape */
        bool hidden;                /*   visibility */
    } cursor;
    struct save_state {             /* saved parameters */
        struct disp_attr disp;
        struct char_attr attr;
        struct cursor cursor;
        char tabstop[MAX_TABSTOPS];
    } saved;
    struct default_state {          /* default parameters */
        struct char_attr attr;
        struct cursor cursor;
    } defaults;
    enum console_state {            /* console control state */
        S_NORM,                     /*   normal */
        S_ESC,                      /*   escape sequence */
        S_CSI                       /*   control sequence */
    } state;
};

/**
 * Writes a character to the console at the current cursor position, then
 * advances the cursor to the next position.
 *
 * @param c the character to write
 */
void con_write(char c);

void con_reset(void);
void con_save(void);
void con_restore(void);
void con_cursor_save(void);
void con_cursor_restore(void);

#endif /* __CONSOLE_H */
