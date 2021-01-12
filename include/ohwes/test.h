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
 *    File: include/ohwes/thunk.h                                             *
 * Created: January 3, 2021                                                   *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#ifndef __TEST_H
#define __TEST_H

#include <stdio.h>

// #define NOANSI  1

#define FAIL    0
#define PASS    1

#define STRINGIFY(x) #x

#define anykey()                            \
do {                                        \
    print("Press any key to continue...");  \
    do { } while (getchar() == '\0');       \
    print("\n");                            \
} while (0)

#define wait()                                                              \
do {                                                                        \
    char __c; while ((__c = getchar()) == '\0');                            \
    if (__c == ' ') break;                                                  \
    if (__c == '\033') goto cancel;                                         \
} while (1)

#define test(n,f)                                                           \
do {                                                                        \
    clear_screen(); print(n "\n");                                          \
    char *__ptr = n; while (*(__ptr++) != 0) putchar(0xCD);                 \
    print("\n\n");                                                          \
    if (!f()) { __failmsg(n); goto done; }                                  \
    __passmsg(n); wait();                                                   \
} while (0)
/*
#define suite_begin(n)                                                      \
do {                                                                        \
    clear_screen(); print("Test Suite: " n "\n\n"); wait();                 \
} while (0)

#define suite_end(n)                                                        \
do {                                                                        \
    clear_screen(); print("Complete: " n "\n\n"); goto done;                \
} while(0)
 */
void reset_console(void);
void save_console(void);
void restore_console(void);
void save_cursor(void);
void restore_cursor(void);
void clear_screen(void);
void print(const char *str);

void start_interactive_tests(void);
void test_vga(void);
void test_ansi(void);

void __failmsg(const char *name);
void __passmsg(const char *name);

#endif /* __TEST_H */