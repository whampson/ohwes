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
 *    File: include/ohwes/debug.h                                             *
 * Created: December 24, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#ifndef __DEBUG_H
#define __DEBUG_H

#define DBGBUFSIZ   256     /* dbgprintf() buffer size */
#define DBGPORT     0xE9    /* dbgprintf() I/O port */

/**
 * Prints a message to the debug console.
 * Some emulators (Bochs) provide a 'debug' I/O port
 * which will display any character written to the port
 * in the emulator's debug console. Useful if you need
 * to print long messages and want the ability to scroll
 * back.
 */
#define dbgprintf(...)                                  \
do {                                                    \
    static char __c, *__ptr, __buf[DBGBUFSIZ];          \
    __ptr = __buf;                                      \
    snprintf(__buf, DBGBUFSIZ, __VA_ARGS__);            \
    while ((__c = *(__ptr++)) != 0) {                   \
        __asm__ volatile (                              \
            "outb %b0, %w1"                             \
            : : "a"(__c), "d"(DBGPORT)                  \
        );                                              \
    }                                                   \
} while (0)

#endif /* __DEBUG_H */
