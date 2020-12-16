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
 *    File: include/nb/nb.h                                                   *
 * Created: December 16, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *                                                                            *
 * Useful non-standard macros safe for use across the entire system.          *
 *============================================================================*/

#ifndef __NB_H
#define __NB_H

/**
 * Returns the greater value.
 * Adapted from https://stackoverflow.com/a/3437484.
 */
#define max(a,b)            \
do {                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : _b;      \
} while (0)

/**
 * Returns the smaller value.
 * Adapted from https://stackoverflow.com/a/3437484.
 */
#define min(a,b)            \
do {                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b;      \
} while (0)

/**
 * Exchanges two values.
 */
#define swap(a,b)           \
do {                        \
    a ^= b;                 \
    b ^= a;                 \
    a ^= b;                 \
} while(0)

/**
 * Checks whether a bit or bitmask is set in a value.
 * 
 * @param x the value to check
 * @param f the flag to check for
 */
#define has_flag(x,f) (((x)&(f))==(f))

/**
 * Case statement fall-through hint (GCC).
 */
#define __fallthrough   __attribute__((fallthrough))

#endif /* __NB_H */
