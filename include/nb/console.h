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
 *    File: include/nb/console.h                                              *
 * Created: December 14, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#ifndef __CONSOLE_H
#define __CONSOLE_H

#include <stdbool.h>
#include <stdint.h>
#include <drivers/vga.h>

#define NUM_CONSOLES        8

/**
 * System Console
 */
struct console
{
    bool initialized;       /* console initalized? */
    int cols, rows;         /* screen dimensions */
    char *framebuf;         /* frame buffer */
    struct disp_attr {      /* display attributes */
        bool blink_on;      /*   character blinking enabled */
        bool invert;        /*   colors are inverted */
        bool conceal;       /*   screen is blanked */
    } disp;
    struct char_attr {      /* character attributes */
        int bg, fg;         /*   background, foreground colors */
        bool bright;        /*   use bright foreground */
        bool faint;         /*   use dim foreground */
        bool underline;     /*   show underline */
        bool blink;         /*   blink character (if enabled) */
    } attr;
    struct cursor {         /* cursor parameters */
        int x, y;           /*   position */
        int shape;          /*   shape */
        bool hidden;        /*   visibility */
    } cursor;
};

/**
 * Initializes the console driver.
 */
void con_init(void);

/**
 * Writes a character to the console at the current cursor position, then 
 * advances the cursor to the next position.
 * 
 * @param c the character to write
 */
void con_write( char c);

#endif /* __CONSOLE_H */
