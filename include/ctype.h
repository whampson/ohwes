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
 *    File: include/ctype.h                                                   *
 * Created: December 14, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *                                                                            *
 * Implementation of ctype.h from the C Standard Library.                     *
 *============================================================================*/

/* Completion Status: DONE */

#ifndef __CTYPE_H
#define __CTYPE_H

static inline int iscntrl(int ch)
{
    return (ch >= 0x00 && ch <= 0x1F) || ch == 0x7F;
}

static inline int isblank(int ch)
{
    return ch == '\t' || ch == ' ';
}

static inline int isspace(int ch)
{
    return ch == '\t'
        || ch == '\f'
        || ch == '\v'
        || ch == '\n'
        || ch == '\r'
        || ch == ' ';
}

static inline int isupper(int ch)
{
    return ch >= 'A' && ch <= 'Z';
}

static inline int islower(int ch)
{
    return ch >= 'a' && ch <= 'z';
}

static inline int isalpha(int ch)
{
    return isupper(ch) || islower(ch);
}

static inline int isdigit(int ch)
{
    return ch >= '0' && ch <= '9';
}

static inline int isxdigit(int ch)
{
    return isdigit(ch)
        || (ch >= 'A' && ch <= 'F')
        || (ch >= 'a' && ch <= 'f');
}

static inline int isalnum(int ch)
{
    return isalpha(ch) || isdigit(ch);
}

static inline int ispunct(int ch)
{
    return (ch >= '!' && ch <= '/')
        || (ch >= ':' && ch <= '@')
        || (ch >= '[' && ch <= '`')
        || (ch >= '{' && ch <= '~');
}

static inline int isgraph(int ch)
{
    return ch >= '!' && ch <= '~';
}

static inline int isprint(int ch)
{
    return ch >= ' ' && ch <= '~';
}

static inline int tolower(int ch)
{
    if (isupper(ch)) {
        ch |= 0x20;
    }

    return ch;
}

static inline int toupper(int ch)
{
    if (islower(ch)) {
        ch &= ~0x20;
    }

    return ch;
}

#endif /* __CTYPE_H */
