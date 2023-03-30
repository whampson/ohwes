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
 *         File: include/stdlib.h
 *      Created: December 18, 2020
 *       Author: Wes Hampson
 *       Module: C Standard Library (C99)
 * =============================================================================
 */

/* Status: INCOMPLETE */

#ifndef __STDLIB_H
#define __STDLIB_H

#include <stdint.h>

#ifndef __SIZE_T_DEFINED
#define __SIZE_T_DEFINED
typedef uint32_t size_t;
#endif

#ifndef __NULL_DEFINED
#define __NULL_DEFINED
#define NULL ((void *) 0)
#endif

static inline char * itoa(int value, char *str, int base)   /* Non-standard */
{
    static char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char tmp[33];
    char *ptr;
    int i, len;
    unsigned v;

    if (base < 2 || base > 36) {
        str[0] = '\0';
        return str;
    }
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return str;
    }

    int sign = (base == 10 && value < 0);
    if (sign) {
        v = -value;
    }
    else {
        v = (unsigned) value;
    }

    ptr = tmp;
    while (v) {
        i = v % base;
        v /= base;
        *ptr = digits[i];
        ptr++;
    }
    if (sign) {
        *(ptr++) = '-';
    }
    *ptr = '\0';

    i = 0;
    len = ptr - tmp;
    while (i < len) {
        str[i] = tmp[len - i - 1];
        i++;
    }
    str[i] = '\0';

    return str;
}

#endif /* __STDLIB_H */
