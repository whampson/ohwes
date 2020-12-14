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

#include <stdint.h>

/**
 * Initializes the console.
 */
void con_init(void);

/**
 * Disables the cursor blink effect.
 */
void blink_off(void);

/**
 * Enables the cursor blink effect.
 */
void blink_on(void);

/**
 * Disables the cursor.
 */
void hide_cursor(void);

/**
 * Enables the cursor.
 */
void show_cursor(void);

/**
 * Gets the current linear cursor position. 
 * A value of 0 represents the top left corner of the display area.
 * 
 * @return the cursor position
 */
uint16_t get_cursor_pos(void);

/**
 * Sets the current linear cursor position. 
 * A value of 0 represents the top left corner of the display area.
 * 
 * @param pos the new linear cursor position
 */
void set_cursor_pos(uint16_t pos);

/**
 * Gets the current cursor shape. 
 * The cursor shape is defined as the area between two scan lines. A scan line 
 * value of 0 represents the top of the current row. The maximum scan line is 
 * determined by the character height (usually 15).
 * 
 * @return the cursor shape, represented as a packed scan line tuple where the
 *         low byte contains the starting scan line and the high byte contains
 *         the ending scan line
 */
uint16_t get_cursor_shape(void);

/**
 * Sets the cursor shape.
 * The cursor shape is defined as the area between two scan lines. A scan line 
 * value of 0 represents the top of the current row. The maximum scan line is 
 * determined by the character height (usually 15).
 *
 * @param start the scan line at which to begin drawing the cursor
 * @param end the scan line at which to stop drawing the cursor
 */
void set_cursor_shape(uint8_t start, uint8_t end);

#endif /* __CONSOLE_H */
