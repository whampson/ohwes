/* =============================================================================
 * Copyright (C) 2020-2023 Wes Hampson. All Rights Reserved.
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
 *         File: kernel/printf.c
 *      Created: Dec 22, 2023
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <console.h>

// inspired by XNU's printf impl:
// https://opensource.apple.com/source/xnu/xnu-201/osfmk/kern/printf.c.auto.html

// printf family spec:
// https://en.cppreference.com/w/c/io/fprintf

int _doprintf(const char *format, va_list *args, void (*putc)(char))
{
    int nwritten = 0;

    #define write(c) \
        (*putc)(c); \
        nwritten++; \

    while (format != NULL && *format != '\0') {
        bool ljustify = false;
        bool signflag = false;
        bool signpad = false;
        bool altflag = false;
        bool zeropad = false;
        register int width = 0;
        register int prec = 1;
        bool default_prec = true;

        //
        // next char
        //
        register char c = *format++;
        if (c != '%') {
            write(c);
            continue;
        }

        //
        // flags
        //
        bool parse_flag = true;
        while (parse_flag && *format != '\0') {
            c = *format++;
            switch (c) {
                case '-': ljustify = true; break;
                case '+': signflag = true; break;
                case ' ': signpad = true; break;
                case '#': altflag = true; break;
                case '0': zeropad = true; break;
                default: parse_flag = false;
            }
        }

        if (signflag && signpad) {
            signpad = false;    // space ignored if + present
        }

        if (ljustify && zeropad) {
            zeropad = false;    // 0 ignored if - present
        }

        (void) altflag;

        //
        // width
        //
        while (isdigit(c)) {
            width *= 10;
            width += (c - '0');
            c = *format++;
        }
        if (c == '*') {
            width = va_arg(*args, int);
            if (width < 0) {    // negative width enables left justify
                width = -width;
                ljustify = true;
            }
            c = *format++;
        }

        //
        // precision
        //
        if (c == '.') {
            default_prec = false;
            prec = 0;
            c = *format++;
            while (isdigit(c)) {
                prec *= 10;
                prec += (c - '0');
                c = *format++;
            }
            if (c == '*') {
                prec = va_arg(*args, int);
                if (prec < 0) { // precision ignored if negative
                    default_prec = true;
                }
                c = *format++;
            }
        }

        //
        // conversion specifier
        //
        switch (c) {
            case '%': {
                write(c);
                break;
            }
            case 'c': {
                unsigned char ch = (unsigned char) va_arg(*args, int);
                write(ch);
                break;
            }
            case 's': {
                const char *str = va_arg(*args, const char*);
                if (str == NULL) {
                    str = "(null)";
                }

                int len = strlen(str);
                if (default_prec) {
                    prec = len;
                }
                else {
                    len = prec;
                }

                if (!ljustify) {
                    while (width-- > prec) {
                        write(' ');
                    }
                }

                while (len-- > 0 && *str != '\0') write(*str++);

                if (ljustify) {
                    while (width-- > prec) {
                        write(' ');
                    }
                }

                continue;
            }
            default: {
                break;
            }
        }

    }

    return nwritten;
}

static char *sprintf_buffer;
static char *sprintf_buffer;
static size_t buffer_size;

static void _sprintf_putc(char c)
{
    *sprintf_buffer++ = c;
    *sprintf_buffer = '\0';
}

static void _snprintf_putc(char c)
{
    if (buffer_size > 0) {
        *sprintf_buffer++ = c;
        *sprintf_buffer = '\0';
        buffer_size--;
    }
}

/**
 * "Writes the results to the output stream stdout."
*/
int printf(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    int nwritten = _doprintf(format, &args, con_write);
    va_end(args);

    return nwritten;
}

/**
 * "Writes the results to a character string buffer. The behavior is undefined if
 * the string to be written (plus the terminating null character) exceeds the
 * size of the array pointed to by buffer."
*/
int sprintf(char *buffer, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    sprintf_buffer = buffer;
    *sprintf_buffer = '\0';
    int nwritten = _doprintf(format, &args, _sprintf_putc);
    va_end(args);

    return nwritten;
}

/**
 * "Writes the results to a character string buffer. At most bufsz - 1
 * characters are written. The resulting character string will be terminated
 * with a null character, unless bufsz is zero. If bufsz is zero, nothing is
 * written and buffer may be a null pointer, however the return value (number of
 * bytes that would be written not including the null terminator) is still
 * calculated and returned."
*/
int snprintf(char *buffer, size_t bufsz, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    sprintf_buffer = buffer;
    *sprintf_buffer = '\0';
    buffer_size = bufsz;
    int nlength = _doprintf(format, &args, _snprintf_putc);
    va_end(args);

    return nlength; // num chars which would've been written if bufsz ignored
}
