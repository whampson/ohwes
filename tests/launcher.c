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
 *    File: tests/launcher.c                                                  *
 * Created: January 3,2021                                                    *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ohwes/io.h>
#include <ohwes/test.h>
#include <ohwes/console.h>
#include <ohwes/keyboard.h>
#include <drivers/vga.h>

static bool end_tests = false;

static void menu_header(void);
static void console_menu(void);

void start_interactive_tests(void)
{
    end_tests = false;
    kbd_setecho(false);

print_menu:
    menu_header();
    print("Test Suite:\n");
    print("  [1] Console\n");
    print("  [2] printf()\n");

    while (1) {
        if (end_tests) {
            goto cleanup;
        }
        switch (getchar()) {
            case '1':
                console_menu();
                goto print_menu;
            case '\033':
                end_tests = true;
                break;
        }
    }

cleanup:
    reset_console();
    kbd_setecho(true);
    print("Exiting Test Mode...\n");
}

static void console_menu(void)
{
print_menu:
    menu_header();
    print("Console Tests:\n");
    print("   [1] VGA Display\n");
    print("   [2] Escape Sequences\n");
    print("   [3] Keyboard\n");

    while (1) {
        switch (getchar()) {
            case '1':
                test_vga();
                goto print_menu;
            case '2':
                test_ansi();
                goto print_menu;
            case '\b':
                return;
            case '\033':
                end_tests = true;
                return;
        }
    }

}

static void menu_header(void)
{
#ifndef NOANSI
    print("\033[44m\033[2J");
    print("\033[2;34H\033[30;47m  ");
#else
    con_reset();
#endif

    print("Test Mode");
#ifndef NOANSI
    print("  \033[37;44m\n");
#endif
    print("\n\n");
    print(" * Type a number to select a test category.\n");
    print(" * Press <BACKSPACE> to return to the previous menu.\n");
    print(" * Press <SPACE> to advance to the next test.\n");
    print(" * Press <ESC> to cancel a test or exit Test Mode.\n");
    print("\n\n");
}

void reset_console(void)
{
#ifdef NOANSI
    con_reset();
#endif
    print("\033c");
}

void save_console(void)
{
#ifdef NOANSI
    con_save();
#endif
    print("\0337");
}

void restore_console(void)
{
#ifdef NOANSI
    con_restore();
#endif
    print("\0338");
}

void save_cursor(void)
{
#ifdef NOANSI
    con_cursor_save();
#endif
    print("\033[s");
}

void restore_cursor(void)
{
#ifdef NOANSI
    con_cursor_restore();
#endif
    print("\033[u");
}

void clear_screen(void)
{
#ifdef NOANSI
    size_t n = VGA_TEXT_COLS * VGA_TEXT_ROWS * sizeof(struct vga_cell);
    memset((void *) VGA_FRAMEBUF_COLOR, 0, n);
#else
    print("\033[2J\033[H");
#endif
}

void print(const char *str)
{
    char c;
    while ((c = *(str++)) != '\0') {
        putchar(c);
    }
}

void __failmsg(const char *name)
{
    printf("\n");
#ifndef NOANSI
    print("\033[31mFAIL\033[37m");
#else
    print("FAIL");
#endif
    print(": "); print(name); print("\n");
}

void __passmsg(const char *name)
{
    printf("\n");
#ifndef NOANSI
    print("\033[32mPASS\033[37m");
#else
    print("PASS");
#endif
    print(": "); print(name); print("\n");
}
