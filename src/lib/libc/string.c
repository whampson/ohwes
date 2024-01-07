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
 *         File: lib/libc/string.c
 *      Created: January 3, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <string.h>

void * memcpy(void * restrict dest, const void * restrict src, size_t count)
{
    char *d = dest;
    const char *s = src;

    while (count--) {
        *d++ = *s++;
    }

    return dest;
}

void * memmove(void *dest, const void *src, size_t count)
{
    char *d = dest;
    const char *s = src;

    if (src == dest) {
        return dest;
    }

    // right overlap, copy forwards from head
    if (dest < src && dest + count > src) {
        while (count--) {
            *d++ = *s++;    // less evil
        }
        return dest;
    }

    // left overlap, copy backwards from tail
    if (src < dest && src + count > dest) {
        d += count; s += count;
        while (count--) {
            *--d = *--s;    // evil
        }
        return dest;
    }

    // no overlap, go fast >:)
    return memcpy(dest, src, count);
}

void * memset(void *dest, char c, size_t count)
{
    char *d = dest;
    while (count--) {
        *d++ = c;
    }

    return dest;
}

int memcmp(const void *lhs, const void *rhs, size_t count)
{
    const char *l = lhs;
    const char *r = rhs;

    while ((*l == *r) && count--) {
        l++; r++;
    }

    return *l - *r;
}

size_t strlen(const char *str)
{
    size_t len = 0;
    while (*str++) {
        len++;
    }

    return len;
}

int strcmp(const char *lhs, const char *rhs)
{
    while (*lhs && (*lhs == *rhs)) {
        lhs++; rhs++;
    }

    return *lhs - *rhs;
}
