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
 *    File: include/ohwes/ohwes.h                                             *
 * Created: December 16, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *                                                                            *
 * Useful macros and functions.                                               *
 *============================================================================*/

#ifndef __OHWES_H
#define __OHWES_H

#include <stdint.h>

/**
 * Checks whether a bit or bitmask is set in a value.
 *
 * @param x the value to check
 * @param f the flag to check for
 */
#define has_flag(x,f) (((x)&(f))==(f))

/**
 * Exchanges two values.
 */
#define swap(a,b)           \
do {                        \
    (a) ^= (b);             \
    (b) ^= (a);             \
    (a) ^= (b);             \
} while(0)

static inline int64_t minb(int8_t a, int8_t b)      { return a < b ? a : b; }
static inline int64_t mins(int16_t a, int16_t b)    { return a < b ? a : b; }
static inline int32_t mini(int32_t a, int32_t b)    { return a < b ? a : b; }
static inline int64_t minl(int64_t a, int64_t b)    { return a < b ? a : b; }
static inline float minf(float a, float b)          { return a < b ? a : b; }
static inline double mind(double a, double b)       { return a < b ? a : b; }
#define min mini

static inline int64_t maxb(int8_t a, int8_t b)      { return a > b ? a : b; }
static inline int64_t maxs(int16_t a, int16_t b)    { return a > b ? a : b; }
static inline int32_t maxi(int32_t a, int32_t b)    { return a > b ? a : b; }
static inline int64_t maxl(int64_t a, int64_t b)    { return a > b ? a : b; }
static inline float maxf(float a, float b)          { return a > b ? a : b; }
static inline double maxd(double a, double b)       { return a > b ? a : b; }
#define max maxi

#endif /* __OHWES_H */
