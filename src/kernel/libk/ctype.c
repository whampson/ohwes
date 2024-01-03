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
 *         File: src/kernel/libk/ctype.c
 *      Created: December 27, 2023
 *       Author: Wes Hampson
 *
 * https://en.cppreference.com/w/c/string/byte
 * =============================================================================
 */

#include <ctype.h>

int isalnum(int c)
{
    return isalpha(c) || isdigit(c);
}

int isalpha(int c)
{
    return isupper(c) || islower(c);
}

int isblank(int c)
{
    return c == '\t' || c == ' ';
}

int iscntrl(int c)
{
    return (c >= 0x00 && c <= 0x1F) || c == 0x7F;
}

int isdigit(int c)
{
    return c >= '0' && c <= '9';
}

int isgraph(int c)
{
    return c >= '!' && c <= '~';
}

int islower(int c)
{
    return c >= 'a' && c <= 'z';
}

int isprint(int c)
{
    return c >= ' ' && c <= '~';
}

int ispunct(int c)
{
    return (c >= '!' && c <= '/')
        || (c >= ':' && c <= '@')
        || (c >= '[' && c <= '`')
        || (c >= '{' && c <= '~');
}

int isspace(int c)
{
    return c == '\t'
        || c == '\f'
        || c == '\v'
        || c == '\n'
        || c == '\r'
        || c == ' ';
}

int isupper(int c)
{
    return c >= 'A' && c <= 'Z';
}

int isxdigit(int c)
{
    return isdigit(c)
        || (c >= 'A' && c <= 'F')
        || (c >= 'a' && c <= 'f');
}


int tolower(int c)
{
    if (isupper(c)) {
        c |= 0x20;
    }

    return c;
}

int toupper(int c)
{
    if (islower(c)) {
        c &= ~0x20;
    }

    return c;
}
