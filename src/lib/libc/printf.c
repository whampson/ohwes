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
 *         File: lib/libc/printf.c
 *      Created: December 22, 2023
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <console.h>

// inspired by XNU's printf impl:
// https://opensource.apple.com/source/xnu/xnu-201/osfmk/kern/printf.c.auto.html

// printf family spec:
// https://en.cppreference.com/w/c/io/fprintf

#define NUM_BUFSIZ 64

int _doprintf(const char *format, va_list *args, void (*putc)(char))
{
    int nwritten = 0;

    #define write(c) \
    do { \
        (*putc)(c); \
        nwritten++; \
    } while(0)

    const char *format_start = format;

    while (format != NULL && *format != '\0')
    {
        bool ljustify = false;
        bool signflag = false;
        bool signpad = false;
        bool altflag = false;
        bool zeropad = false;
        bool capital = false;
        bool negative = false;
        bool signd = false;
        bool zero = false;
        bool default_prec = true;
        register int prec = 1;
        register int width = 0;
        register int radix = 10;
        register int len = 0;
        register char c = 0;
        register char *p = NULL;
        char sign_char = 0;
        char buf[NUM_BUFSIZ];
        uintmax_t num = 0;

        //
        // next char
        //
        c = *format++;
        if (c != '%') {
            write(c);
            continue;
        }

        format_start = format;

        //
        // flags
        //
        bool parse = true;
        while (parse && *format != '\0') {
            c = *format++;
            switch (c) {
                case '-': ljustify = true; break;
                case '+': signflag = true; break;
                case ' ': signpad = true; break;
                case '#': altflag = true; break;
                case '0': zeropad = true; break;
                default: parse = false;
            }
        }

        if (signflag && signpad) {
            signpad = false;    // space ignored if + present
        }

        if (ljustify && zeropad) {
            zeropad = false;    // 0 ignored if - present
        }

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
                    prec = 1;
                }
                c = *format++;
            }
        }

        //
        // length modifier
        //
        enum {
            L_DEFAULT,  // no length specified
            L_HH,       // 'hh',byte
            L_H,        // 'h', short
            L_L,        // 'l', long
            L_LL,       // 'll',long long
            L_J,        // 'j', intmax_t
            L_Z,        // 'z', size_t
            L_T         // 't', ptrdiff_t
        };
        int length = L_DEFAULT;

        parse = true;
        while (parse) {
            bool match = true;
            switch (c) {
                case 'h':
                    if (length == L_DEFAULT) {
                        length = L_H;
                    }
                    else if (length == L_H) {
                        length = L_HH;
                        parse = false;
                    }
                    break;
                case 'l':
                    if (length == L_DEFAULT) {
                        length = L_L;
                    }
                    else if (length == L_L) {
                        length = L_LL;
                        parse = false;
                    }
                    break;
                case 'j':
                    if (length == L_DEFAULT) {
                        length = L_J;
                        parse = false;
                    }
                    break;
                case 'z':
                    if (length == L_DEFAULT) {
                        length = L_Z;
                        parse = false;
                    }
                    break;
                case 't':
                    if (length == L_DEFAULT) {
                        length = L_T;
                        parse = false;
                    }
                    break;
                default:
                    parse = false;
                    match = false;
                    break;
            }

            if (match) {
                c = *format++;
            }
        }

        //
        // conversion specifier
        //
        switch (c)
        {
            //
            // strings: write then continue to top of loop
            //
            default: {
                write('%');         // abort! just write the format string
                while (format_start < format) {
                    write(*format_start++);
                }
                continue;
            }
            case '%': {
                write(c);
                continue;
            }
            case 'c': {
                write((char) va_arg(*args, int));
                continue;
            }
            case 's': {
                if (length == L_DEFAULT) {
                    const char *str = va_arg(*args, const char*);
                    if (str == NULL) {
                        str = "(null)";
                    }

                    len = strlen(str);
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
                }
                else if (length == L_L) {
                    // TODO: wchar_t support
                }
                continue;
            }

            //
            // numerics: set params then write below
            //
            case 'o': {
                radix = 8;
                goto get_unsigned;
            }
            case 'X': {
                capital = true;
                __fallthrough;
            }
            case 'x': {
                radix = 16;
                goto get_unsigned;
            }
            case 'd': __fallthrough;
            case 'i': {
                signd = true;
                intmax_t n = 0;
                switch (length) {
                    default:    n = va_arg(*args, int); break;
                    case L_HH:  n = va_arg(*args, signed char); break;
                    case L_H:   n = va_arg(*args, short); break;
                    case L_L:   n = va_arg(*args, long); break;
                    case L_LL:  n = va_arg(*args, long long); break;
                    case L_J:   n = va_arg(*args, intmax_t); break;
                    case L_Z:   n = va_arg(*args, size_t); break;
                    case L_T:   n = va_arg(*args, ptrdiff_t); break;
                }
                if (n < 0) {
                    negative = true;
                    n = -n;
                }
                num = n;    // store unsigned
                break;
            }
            case 'u': {
            get_unsigned:
                switch (length) {
                    default:    num = va_arg(*args, unsigned int); break;
                    case L_HH:  num = va_arg(*args, unsigned char); break;
                    case L_H:   num = va_arg(*args, unsigned short); break;
                    case L_L:   num = va_arg(*args, unsigned long); break;
                    case L_LL:  num = va_arg(*args, unsigned long long); break;
                    case L_J:   num = va_arg(*args, uintmax_t); break;
                    case L_Z:   num = va_arg(*args, size_t); break;
                    case L_T:   num = va_arg(*args, ptrdiff_t); break;
                }
                break;
            }
        }

        static char digits[]     = "0123456789abcdefghijklmnopqrstuvwxyz";
        static char digits_cap[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        zero = (num == 0);

        // convert num to string
        p = &buf[NUM_BUFSIZ-1];
        while (num) {
            if (capital) {
                *p-- = digits_cap[num % radix];
            }
            else {
                *p-- = digits[num % radix];
            }
            num /= radix;
        }
        len = &buf[NUM_BUFSIZ] - (p+1);

        // count the number of zeros needed for precision
        // keep track of total string length
        int num_zeros = 0;
        while (prec > len) {
            num_zeros++;
            len++;
        }

        // determine sign char, tally new length
        if (signd) {
            if (negative) {
                sign_char = '-';
                len++;
            }
            else if (signflag) {
                sign_char = '+';
                len++;
            }
            else if (signpad) {
                sign_char = ' ';
                len++;
            }
        }

        // collect length for alternative representation (#)
        if (altflag) {
            if (radix == 8 && num_zeros == 0) {
                len++;
                num_zeros++;
            }
            else if (radix == 16 && !zero) {
                len += 2;
            }
        }

        //
        // number printing
        //

        // handle right justification
        if (!ljustify) {
            if (zeropad && default_prec) {
                while (width > len) { num_zeros++; len++; }
            }
            else {
                while (width > len) { width--; write(' '); }       // spaces always come first...
            }
        }

        // write sign char
        if (sign_char) {
            write(sign_char);                               // followed by the sign...
        }

        // write any radix prefixes
        if (altflag) {
            if (radix == 16 && !zero) {
                write('0');
                write((capital) ? 'X' : 'x');           // then the radix prefix (0x etc)...
            }
        }

        // write any leading zeros
        while (num_zeros-- > 0) {
            write('0');                                 // then any leading zeros...
        }

        // write stringifed number
        while (++p != &buf[NUM_BUFSIZ]) write(*p);      // next, the number itself...

        // write padding for left justify
        if (ljustify) {
            while (width > len) {
                width--;
                write(' ');                             // and finally, trailing spaces.
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
    int nwritten = _doprintf(format, &args, console_write);
    va_end(args);

    return nwritten;
}

int vprintf(const char *format, va_list args)
{
    return _doprintf(format, &args, console_write);
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