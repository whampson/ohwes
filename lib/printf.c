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
 *    File: lib/printf.c                                                      *
 * Created: December 16, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *                                                                            *
 * An implementation of the printf-family functions.                          *
 *============================================================================*/

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ohwes/io.h>
#include <ohwes/ohwes.h>

struct printf_params
{
    /* I/O parameters */
    const char *fmt;            /* format specifier */
    va_list args;               /* format args */
    int fd;                     /* file descriptor */
    bool use_buf;               /* write to buffer instead of file */
    bool bounded;               /* is the buffer bounded by buflen? */
    char *buf;                  /* output buffer */
    size_t n;                   /* output buffer size */
    size_t pos;                 /* output buffer position */
    
    /* format specifier optional fields */
    int flags;
    int width;
    int precision;
    int length;

    /* integer formatting params */
    bool int_signed;
    bool int_upper;
    int int_radix;
    bool int_pointer;

    /* parsing state */
    bool parsing;
    int field;
};

enum printf_flags
{
    F_NONE      = (0),
    F_LJUST     = (1 << 0),     /* '-'  left-justify */
    F_SIGN      = (1 << 1),     /* '+'  always print sign */
    F_SIGNPAD   = (1 << 2),     /* ' '  print space if no sign */
    F_PREFIXDOT = (1 << 3),     /* '#'  print radix prefix or decimal point */
    F_ZEROPAD   = (1 << 4),     /* '0'  left-pad with zeros */
};

enum printf_lengths
{
    L_NONE  = 0,                /* int */
    L_HH    = ('h'<<8)|'h',     /* char */
    L_H     = 'h',              /* short int */
    L_L     = 'l',              /* long int */
    L_LL    = ('l'<<8)|'l',     /* long long int */
    L_J     = 'j',              /* intmax_t */
    L_Z     = 'z',              /* size_t */
    L_T     = 't',              /* ptrdiff_t */
    L_BIGL  = 'L'               /* long double */
};

enum printf_field
{
    S_FLAGS,                    /* reading flags */
    S_WIDTH,                    /* reading width */
    S_PRECISION,                /* reading precision */
};

#define DEFAULT_WIDTH       0
#define DEFAULT_PRECISION   (-1)

static int _printf(struct printf_params *p);
static int fmt_int(struct printf_params *p);
static int fmt_char(struct printf_params *p);
static int fmt_string(struct printf_params *p);
static int pad(struct printf_params *p, int n, char c);
static int write_string(struct printf_params *p, char *str, size_t len);
static int write_char(struct printf_params *p, char c);
static void reset_fmt_params(struct printf_params *p);

int printf(const char *fmt, ...)
{
    int retval;
    va_list ap;

    va_start(ap, fmt);
    retval = vprintf(fmt, ap);
    va_end(ap);

    return retval;
}

int sprintf(char *str, const char *fmt, ...)
{
    int retval;
    va_list ap;

    va_start(ap, fmt);
    retval = vsprintf(str, fmt, ap);
    va_end(ap);

    return retval;
}

int snprintf(char *str, size_t n, const char *fmt, ...)
{
    int retval;
    va_list ap;

    va_start(ap, fmt);
    retval = vsnprintf(str, n, fmt, ap);
    va_end(ap);

    return retval;
}

int vprintf(const char *fmt, va_list args)
{
    struct printf_params p = { 0 };
    p.fmt = fmt;
    p.args = args;
    p.fd = STDOUT_FILENO;
    p.use_buf = false;

    return _printf(&p);
}

int vsprintf(char *str, const char *fmt, va_list args)
{
    struct printf_params p = { 0 };
    p.fmt = fmt;
    p.args = args;
    p.fd = STDOUT_FILENO;
    p.use_buf = true;
    p.buf = str;
    p.bounded = false;

    return _printf(&p);
}

int vsnprintf(char *str, size_t n, const char *fmt, va_list args)
{
    struct printf_params p = { 0 };
    p.fmt = fmt;
    p.args = args;
    p.fd = STDOUT_FILENO;
    p.use_buf = true;
    p.buf = str;
    p.bounded = true;
    p.n = n;

    return _printf(&p);
}

static void reset_fmt_params(struct printf_params *p)
{
    p->parsing = true;
    p->field = S_FLAGS;
    p->flags = F_NONE;
    p->width = DEFAULT_WIDTH;
    p->precision = DEFAULT_PRECISION;
    p->length = L_NONE;
    p->int_pointer = false;
    p->int_radix = 10;
    p->int_signed = true;
    p->int_upper = false;
}

static int _printf(struct printf_params *p)
{
    char *f = (char *) p->fmt;
    int nchars = 0;
    char c;

    while ((c = *(f++)) != '\0') {
        switch (c)
        {
            case '%':   /* begin parsing */
                if (!p->parsing) {
                    reset_fmt_params(p);
                    continue;
                }
                goto sendchar;
            case '.':   /* begin precision field */
                if (!p->parsing) goto sendchar;
                p->field = S_PRECISION;
                p->precision = 0;
                continue;
            case '*':   /* get parameter from arg list */
                if (!p->parsing) goto sendchar;
                if (p->field == S_FLAGS) {
                    p->field = S_WIDTH;
                }
                if (p->field == S_WIDTH) {
                    p->width = va_arg(p->args, int);
                }
                if (p->field == S_PRECISION) {
                    p->precision = va_arg(p->args, int);
                }
                continue;
            
            /* flags */
            case '-':   /* left-justify flag */
                if (!p->parsing) goto sendchar;
                if (p->field == S_FLAGS) p->flags |= F_LJUST;
                continue;
            case '+':   /* sign flag */
                if (!p->parsing) goto sendchar;
                if (p->field == S_FLAGS) p->flags |= F_SIGN;
                continue;
            case ' ':   /* sign pad flag */
                if (!p->parsing) goto sendchar;
                if (p->field == S_FLAGS) p->flags |= F_SIGNPAD;
                continue;
            case '#':   /* hex prefix/decimal point flag */
                if (!p->parsing) goto sendchar;
                if (p->field == S_FLAGS) p->flags |= F_PREFIXDOT;
                continue;
            case '0':   /* left-pad with zeros flag */
                if (!p->parsing) goto sendchar;
                if (p->field == S_FLAGS) p->flags |= F_ZEROPAD;
                if (p->field != S_FLAGS) goto digit;
                continue;
            
            /* length specifiers */
            case 'h':   /* half-word, half half-word */
                if (!p->parsing) goto sendchar;
                if (p->length == L_H) p->length = L_HH;
                else p->length = L_H;
                continue;
            case 'l':   /* long */
                if (!p->parsing) goto sendchar;
                p->length = L_L;
                continue;
            case 'z':   /* size_t */
                if (!p->parsing) goto sendchar;
                p->length = L_Z;
                continue;
            case 't':   /* ptrdiff_t */
                if (!p->parsing) goto sendchar;
                p->length = L_T;
                continue;
            
            /* format specifiers */
            case 'd':
            case 'i':   /* signed decimal integer */
                if (!p->parsing) goto sendchar;
                p->int_signed = true;
                p->int_upper = false;
                p->int_radix = 10;
                nchars += fmt_int(p);
                continue;
            case 'u':   /* unsigned decimal integer */
                if (!p->parsing) goto sendchar;
                p->int_signed = false;
                p->int_upper = false;
                p->int_radix = 10;
                nchars += fmt_int(p);
                continue;
            case 'o':   /* unsigned octal integer */
                if (!p->parsing) goto sendchar;
                p->int_signed = false;
                p->int_upper = false;
                p->int_radix = 8;
                nchars += fmt_int(p);
                continue;
            case 'x':   /* unsigned hexadecimal integer */
                if (!p->parsing) goto sendchar;
                p->int_signed = false;
                p->int_upper = false;
                p->int_radix = 16;
                nchars += fmt_int(p);
                continue;
            case 'X':   /* unsigned hexadecimal integer, uppercase */
                if (!p->parsing) goto sendchar;
                p->int_signed = false;
                p->int_upper = true;
                p->int_radix = 16;
                nchars += fmt_int(p);
                continue;
            case 'p':   /* pointer address */
                if (!p->parsing) goto sendchar;
                p->int_pointer = true;
                p->length = L_Z;    /* treat as size_t */
                nchars += fmt_int(p);
                continue;
            case 'c':   /* character */
                if (!p->parsing) goto sendchar;
                nchars += fmt_char(p);
                continue;
            case 's':   /* string */
                if (!p->parsing) goto sendchar;
                nchars += fmt_string(p);
                continue;
            
            default:
                if (!p->parsing) goto sendchar;
            digit:
                if (isdigit(c)) {
                    if (p->field == S_FLAGS) {
                        p->field = S_WIDTH;
                    }
                    if (p->field == S_WIDTH) {
                        p->width *= 10;
                        p->width += (c - '0');
                    }
                    if (p->field == S_PRECISION) {
                        p->precision *= 10;
                        p->precision += (c - '0');
                    }
                }
                continue;
        }

    sendchar:
        nchars += write_char(p, c);
    }

    return nchars;
}

static int fmt_int(struct printf_params *p)
{
    intmax_t value;
    char buf[33];
    int i;
    int len;
    int nchars;
    int nprec;
    int npad;
    char lpadchar;

    p->parsing = false;

    /* special formatting for pointers */
    if (p->int_pointer) {
        p->int_radix = 16;
        p->int_signed = false;
        p->int_upper = false;
        if (!p->width && !p->flags && p->precision < 0) {
            p->precision = 8;
        }
    }

    /* extract value according to length specifier */
    switch (p->length) {
        case L_NONE:
            value = (p->int_signed)
                ? (intmax_t) va_arg(p->args, int)
                : (intmax_t) va_arg(p->args, unsigned int);
            break;
        case L_HH:
            value = (p->int_signed)
                ? (intmax_t) va_arg(p->args, signed char)
                : (intmax_t) va_arg(p->args, unsigned char);
            break;
        case L_H:
            value = (p->int_signed)
                ? (intmax_t) va_arg(p->args, short int)
                : (intmax_t) va_arg(p->args, unsigned short int);
            break;
        case L_L:
            value = (p->int_signed)
                ? (intmax_t) va_arg(p->args, long int)
                : (intmax_t) va_arg(p->args, unsigned long int);
            break;
        case L_LL:
            value = (p->int_signed)
                ? (intmax_t) va_arg(p->args, long long int)
                : (intmax_t) va_arg(p->args, unsigned long long int);
            break;
        case L_J:
            value = (p->int_signed)
                ? (intmax_t) va_arg(p->args, intmax_t)
                : (intmax_t) va_arg(p->args, uintmax_t);
            break;
        case L_Z:
            value = (intmax_t) va_arg(p->args, size_t);
            break;
        case L_T:
            value = (intmax_t) va_arg(p->args, ptrdiff_t);
            break;
    }

    /* get number as string according to radix */
    nchars = 0;
    itoa(value, buf, p->int_radix);

    /* don't print anything if value and precision are both 0 */
    if (value == 0 && p->precision == 0) {
        buf[0] = '\0';
    }
    len = strlen(buf);

    /* convert to uppercase if requested */
    if (p->int_radix == 16 && p->int_upper) {
        for (i = 0; i < len; i++) buf[i] = toupper(buf[i]);
    }

    /* no prefix if value is zero */
    if (value == 0 && has_flag(p->flags, F_PREFIXDOT)) {
        p->flags &= ~F_PREFIXDOT;
    }

    /* determine the number of leading zeroes and spaces to print */
    nprec = p->precision - len;
    if (nprec < 0) nprec = 0;
    npad = p->width - nprec - len;

    /* if sign to be printed, subtract from padding */
    if (value >= 0 && p->int_radix == 10) {
        if (has_flag(p->flags, F_SIGN) || has_flag(p->flags, F_SIGNPAD)) {
            npad -= 1;
        }
    }

    /* if prefix to be printed, subtract from padding */
    if (has_flag(p->flags, F_PREFIXDOT)) {
        if (p->int_radix == 8) npad -= 1;
        if (p->int_radix == 16) npad -= 2;
    }

    /* determine left-pad character */
    if (!has_flag(p->flags, F_LJUST)) {
        if (has_flag(p->flags, F_ZEROPAD) && p->precision <= 0) {
            lpadchar = '0';
        }
        else {
            lpadchar = ' ';
        }
    }

    /* print left-pad (width, spaces) */
    if (lpadchar == ' ') nchars += pad(p, npad, ' ');

    /* print sign */
    if (value >= 0 && p->int_radix == 10) {
        if (has_flag(p->flags, F_SIGN)) {
            nchars += write_char(p, '+');
        }
        else if (has_flag(p->flags, F_SIGNPAD)) {
            nchars += write_char(p, ' ');
        }
    }

    /* print prefix */
    if (has_flag(p->flags, F_PREFIXDOT) && p->int_radix != 10) {
        nchars += write_char(p, '0');
        if (p->int_radix == 16) nchars += write_char(p, (p->int_upper)?'X':'x');
    }

    /* print left-pad (width, zeroes) */
    if (lpadchar == '0') nchars += pad(p, npad, '0');

    /* print left-pad (precision, zeroes) */
    nchars += pad(p, nprec, '0');

    /* print number */
    nchars += write_string(p, buf, len);

    /* print right-pad (left-justify, width, spaces) */
    if (has_flag(p->flags, F_LJUST)) {
        nchars += pad(p, npad, ' ');
    }

    return nchars;
}

static int fmt_char(struct printf_params *p)
{
    int nchars;
    int npad;
    char c;

    p->parsing = false;
    c = (char) va_arg(p->args, int);
    nchars = 0;

    npad = p->width - 1;
    if (!has_flag(p->flags, F_LJUST)) nchars += pad(p, npad, ' ');
    nchars += write_char(p, c);
    if (has_flag(p->flags, F_LJUST)) nchars += pad(p, npad, ' ');

    return nchars;
}

static int fmt_string(struct printf_params *p)
{
    int nchars;
    int npad;
    char *s;
    int len;

    p->parsing = false;

    /* a precision of zero prints an empty string */
    if (p->precision == 0) {
        return 0;
    }

    /* extract string */
    nchars = 0;
    s = va_arg(p->args, char*);
    len = strlen(s);

    /* apply precision as max length */
    if (p->precision > 0) {
        len = p->precision;
    }
    npad = p->width - len;

    /* left-pad with spaces if left-justify flag not set */
    if (!has_flag(p->flags, F_LJUST) && npad > 0 && p->width > 0) {
        nchars += pad(p, npad, ' ');
    }

    /* write the string */
    nchars += write_string(p, s, len);

    /* right-pad with spaces if left-justify flag set */
    if (has_flag(p->flags, F_LJUST) && npad > 0 && p->width > 0) {
        nchars += pad(p, npad, ' ');
    }

    return nchars;
}

static int pad(struct printf_params *p, int n, char c)
{
    int nchars;
    for (nchars = 0; nchars < n; nchars++) {
        write_char(p, c);
    }

    return nchars;
}

static int write_string(struct printf_params *p, char *str, size_t len)
{
    size_t nchars = 0;
    while (nchars < len && *str != '\0') {
        nchars += write_char(p, *(str++));
    }

    return (int) nchars;
}

static int write_char(struct printf_params *p, char c)
{
    if (p->use_buf) {
        if (p->bounded) {
            if (p->n > 0 && p->pos < p->n - 1) {
                p->buf[p->pos++] = c;
                return sizeof(char);
            }
            return 0;
        }
        p->buf[p->pos++] = c;
        return sizeof(char);
    }

    return write(p->fd, &c, sizeof(char));
}
