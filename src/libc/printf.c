/* =============================================================================
 * Copyright (C) 2020-2025 Wes Hampson. All Rights Reserved.
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
 *         File: src/libc/printf.c
 *      Created: December 22, 2023
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ohwes.h>

// inspired by XNU's printf impl:
// https://opensource.apple.com/source/xnu/xnu-201/osfmk/kern/printf.c.auto.html

// printf family spec:
// https://en.cppreference.com/w/c/io/fprintf

#define PRINTF_BUFFER_SIZE  BUFSIZ

struct printf_state;
typedef int (*putc_fn)(struct printf_state *, char);

struct printf_state
{
    char *buf;          // printf buffer
    char *ptr;          // buffer pointer
    size_t bufsz;       // num chars in buffer
    putc_fn putc;       // write character function
};

static struct printf_state make_printf_state(char *buf, size_t bufsz, putc_fn putc)
{
    struct printf_state state = { };
    state.buf = buf;
    state.bufsz = bufsz;
    state.ptr = buf;
    state.putc = putc;

    return state;
}

static int _putc_stdout(struct printf_state *state, char c)
{
    (void) state;
    return write(STDOUT_FILENO, &c, 1);     // TODO: INCREDIBLY INEFFICIENT if write() is not buffering
}

static int _putc_buffer(struct printf_state *state, char c)
{
    if (c == '\0') {
        return 0;   // no char written
    }

    if (state->ptr && state->buf && (
        state->bufsz == 0 ||    // NOTE: can write arbitrary ptr if bufsz=0 !!
        state->ptr - state->buf < state->bufsz - 1))
    {
        *state->ptr++ = c;
        *state->ptr = '\0';
    }

    return 1;   // char was written or would've been written
}

static int _doprintf(const char *fmt, va_list args, struct printf_state *state);

/**
 * "Writes the results to the output stream stdout."
*/
int printf(const char *fmt, ...)
{
    return VPRINT_CALL(fmt, vprintf);
}

/**
 * "Writes the results to a character string buffer. The behavior is undefined if
 * the string to be written (plus the terminating null character) exceeds the
 * size of the array pointed to by buffer."
*/
int sprintf(char *buf, const char *fmt, ...)
{
    return VPRINT_CALL_V(fmt, vsprintf, buf);
}

/**
 * "Writes the results to a character string buffer. At most bufsz - 1
 * characters are written. The resulting character string will be terminated
 * with a null character, unless bufsz is zero. If bufsz is zero, nothing is
 * written and buffer may be a null pointer, however the return value (number of
 * bytes that would be written not including the null terminator) is still
 * calculated and returned."
*/
int snprintf(char *buffer, size_t bufsz, const char *fmt, ...)
{
    return VPRINT_CALL_V(fmt, vsnprintf, buffer, bufsz);
}

int vprintf(const char *fmt, va_list args)
{
    if (fmt == NULL) {
        return -EINVAL;
    }

    struct printf_state state;
    state = make_printf_state(NULL, 0, _putc_stdout);
    return _doprintf(fmt, args, &state);
}

int vsprintf(char *buf, const char *fmt, va_list args)
{
#if KERNEL_BUILD
    pr_warn("%s(%d): sprintf used\n", __func__, __LINE__);
#endif

    if (fmt == NULL || buf == NULL) {
        return -EINVAL;
    }

    struct printf_state state;
    state = make_printf_state(buf, 0, _putc_buffer);
    return _doprintf(fmt, args, &state);
}

int vsnprintf(char *buf, size_t bufsz, const char *fmt, va_list args)
{
    if (fmt == NULL || (buf == NULL && bufsz != 0)) {
        return -EINVAL;
    }

    struct printf_state state;
    state = make_printf_state(buf, bufsz, _putc_buffer);
    return _doprintf(fmt, args, &state);
}

static int _doprintf(const char *fmt, va_list args, struct printf_state *state)
{
    int nwritten = 0;

    // where the magic happens

#define _putchar(c) \
do { \
    int __ret = (*state->putc)(state, c); \
    if (__ret < 0) { \
        goto done; \
    } \
    nwritten += __ret; \
} while(0)

    while (fmt != NULL && *fmt != '\0')
    {
        char c = *fmt++;
        if (c != '%') {
            _putchar(c);
            continue;
        }

        const char *fmt_start = fmt;
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
        int prec = 1;
        int width = 0;
        int radix = 10;
        int len = 0;
        char *p = NULL;
        char sign_char = 0;
        uintmax_t num = 0;

        //
        // flags
        //
        bool parse = true;
        while (parse && *fmt != '\0') {
            c = *fmt++;
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
            c = *fmt++;
        }
        if (c == '*') {
            width = va_arg(args, int);
            if (width < 0) {    // negative width enables left justify
                width = -width;
                ljustify = true;
            }
            c = *fmt++;
        }

        //
        // precision
        //
        if (c == '.') {
            default_prec = false;
            prec = 0;
            c = *fmt++;
            while (isdigit(c)) {
                prec *= 10;
                prec += (c - '0');
                c = *fmt++;
            }
            if (c == '*') {
                prec = va_arg(args, int);
                if (prec < 0) { // precision ignored if negative
                    default_prec = true;
                    prec = 1;
                }
                c = *fmt++;
            }
        }

        enum {  // length specifiers
            L_DEFAULT,  // no length specified
            L_HH,       // 'hh',byte
            L_H,        // 'h', short
            L_L,        // 'l', long
            L_LL,       // 'll',long long
            L_J,        // 'j', intmax_t
            L_Z,        // 'z', size_t
            L_T,        // 't', ptrdiff_t
            L_P,        // (implied by %p)
        };

        //
        // length modifier
        //
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
                c = *fmt++;
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
            default: {  // invalid conversion char:
                _putchar('%');  // abort! just write the fmt string
                while (fmt_start < fmt) {
                    _putchar(*fmt_start++);
                }
                continue;
            }
            case '%': {
                _putchar(c);
                continue;
            }
            case 'c': {
                _putchar((char) va_arg(args, int));
                continue;
            }
            case 's': {
                if (length == L_DEFAULT) {
                    const char *str = va_arg(args, const char*);
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
                            _putchar(' ');
                        }
                    }

                    while (len-- > 0 && *str != '\0') {
                        _putchar(*str++);
                    }

                    if (ljustify) {
                        while (width-- > prec) {
                            _putchar(' ');
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
                radix = 16;
                goto get_unsigned;
            }
            case 'x': {
                radix = 16;
                goto get_unsigned;
            }
            case 'p': {
                radix = 16;
                zeropad = true;
                length = L_P;
                prec = sizeof(void*) << 1;
                goto get_unsigned;
            }
            case 'd': __fallthrough;
            case 'i': {
                signd = true;
                intmax_t n = 0;
                switch (length) {
                    default:    n = va_arg(args, int); break;
                    case L_HH:  n = (signed char)  va_arg(args, int); break;
                    case L_H:   n = (signed short) va_arg(args, int); break;
                    case L_L:   n = va_arg(args, long); break;
                    case L_LL:  n = va_arg(args, long long); break;
                    case L_J:   n = va_arg(args, intmax_t); break;
                    case L_Z:   n = va_arg(args, size_t); break;
                    case L_T:   n = va_arg(args, ptrdiff_t); break;
                    case L_P:   n = va_arg(args, intptr_t); break;
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
                    default:    num = va_arg(args, unsigned int); break;
                    case L_HH:  num = (unsigned char)  va_arg(args, unsigned int); break;
                    case L_H:   num = (unsigned short) va_arg(args, unsigned int); break;
                    case L_L:   num = va_arg(args, unsigned long); break;
                    case L_LL:  num = va_arg(args, unsigned long long); break;
                    case L_J:   num = va_arg(args, uintmax_t); break;
                    case L_Z:   num = va_arg(args, size_t); break;
                    case L_T:   num = va_arg(args, ptrdiff_t); break;
                    case L_P:   num = va_arg(args, uintptr_t); break;
                }
                break;
            }
        }

        zero = (num == 0);

        // convert num to string

        static char digits[]     = "0123456789abcdefghijklmnopqrstuvwxyz";
        static char digits_cap[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        char num2str[64];

        p = &num2str[sizeof(num2str)-1];
        while (num) {
            if (capital) {
                *p-- = digits_cap[num % radix];
            }
            else {
                *p-- = digits[num % radix];
            }
            num /= radix;
        }

        len = &num2str[sizeof(num2str)] - (p+1);


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
                while (width > len) {
                    width--; _putchar(' ');   // spaces always come first...
                }
            }
        }

        // write sign char
        if (sign_char) {
            _putchar(sign_char);              // followed by the sign...
        }

        // write any radix prefixes
        if (altflag) {
            if (radix == 16 && !zero) {
                _putchar('0');
                _putchar((capital)?'X':'x');  // then the radix prefix...
            }
        }

        // write any leading zeros
        while (num_zeros-- > 0) {
            _putchar('0');                    // then any leading zeros...
        }

        // write stringifed number
        while (++p != &num2str[sizeof(num2str)]) {
            _putchar(*p);                     // next, the number itself...
        }

        // write padding for left justify
        if (ljustify) {
            while (width > len) {
                width--;
                _putchar(' ');                // and finally, trailing spaces.
            }
        }
    }
    _putchar('\0');

#undef _putchar

done:
    return nwritten;
}
