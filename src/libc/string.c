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
 *         File: src/libc/string.c
 *      Created: January 3, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

static char _strerr_buf[32];

char * strerror(int errnum)
{
    switch (errnum) {
        case 0:       return "Success";
        case EAGAIN:  return "Resource unavailable, try again";
        case EBADF:   return "Bad file descriptor";
        case EBADRQC: return "Invalid request descriptor";
        case EBUSY:   return "Device or resource busy";
        case EFAULT:  return "Bad address";
        case EINVAL:  return "Invalid argument";
        case EIO:     return "Input/output error";
        case EMFILE:  return "Too many files open in process";
        case ENFILE:  return "Too many files open in system";
        case ENODEV:  return "No such device";
        case ENOENT:  return "No such file or directory";
        case ENOMEM:  return "Not enough memory";
        case ENOSYS:  return "Function not implemented";
        case ENOTTY:  return "Invalid I/O control operation";
        case ENXIO:   return "No such device or address";
        case EPERM:   return "Operation not permitted";
    }

    snprintf(_strerr_buf, sizeof(_strerr_buf), "Unknown error %d\n", errnum);
    return _strerr_buf;
}

void * memcpy(void *restrict dst, const void * restrict src, size_t count)
{
    // TODO: could be optimized to copy DWORDs...
    //       or use __builtin_memcpy?

    char *d = dst;
    const char *s = src;

    while (count--) {
        *d++ = *s++;
    }

    return dst;
}

void * mempcpy(void *restrict dst, const void * restrict src, size_t count)
{
    memcpy(dst, src, count);
    return &((char *) dst)[count];
}


void * memmove(void *dst, const void *src, size_t count)
{
    char *d = dst;
    const char *s = src;

    if (src == dst) {
        return dst;
    }

    // left overlap, copy forwards from head
    if (dst < src && dst + count > src) {
        while (count--) {
            *d++ = *s++;
        }

        return dst;
    }

    // right overlap, copy backwards from tail
    else if (src < dst && src + count > dst) {
        d += count; s += count;
        while (count--) {
            *--d = *--s;    // evil
        }
        return dst;
    }

    // no overlap
    return memcpy(dst, src, count);
}

void * memset(void *dst, int c, size_t count)
{
    char *d = dst;
    while (count--) {
        *d++ = (char) c;
    }

    return dst;
}

int memcmp(const void *lhs, const void *rhs, size_t count)
{
    const char *l = lhs;
    const char *r = rhs;

    if (count == 0) {
        return 0;
    }

    while ((*l == *r) && --count) {
        l++; r++;
    }

    return *l - *r;
}

char * strcpy(char *restrict dst, const char *restrict src)
{
    stpcpy(dst, src);
    return dst;
}

char * stpcpy(char *restrict dst, const char *restrict src)
{
    // https://man7.org/linux/man-pages/man3/strcat.3.html

    char *p = mempcpy(dst, src, strlen(src));
    *p = '\0';
    return p;
}

char * strncpy(char *restrict dst, const char *restrict src, size_t count)
{
    stpncpy(dst, src, count);
    return dst;
}

char * stpncpy(char *restrict dst, const char *restrict src, size_t count)
{
    // https://man7.org/linux/man-pages/man3/strncpy.3.html

    size_t len = strnlen(src, count);
    return memset(mempcpy(dst, src, len), 0, count - len);
}

size_t strlen(const char *str)
{
    size_t len = 0;
    while ((*str++) != '\0') {
        len++;
    }

    return len;
}

size_t strnlen(const char *str, size_t maxlen)
{
    size_t len = 0;
    while (maxlen-- && (*str++) != '\0') {
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

int strncmp(const char *lhs, const char *rhs, size_t count)
{
    if (count == 0) {
        return 0;
    }

    int diff = 0;
    for (int i = 0; i < count; i++, lhs++, rhs++) {
        diff = (*lhs - *rhs);
        if (diff != 0 || !(*lhs) || !(*rhs)) {
            break;
        }
    }

    return diff;
}

char * strcat(char *restrict dst, const char *restrict src)
{
    // https://man7.org/linux/man-pages/man3/strcat.3.html

    stpcpy(dst + strlen(dst), src);
    return dst;
}

char * strncat(char *restrict dst, const char *restrict src, size_t count)
{
    // https://man7.org/linux/man-pages/man3/strncat.3.html

    stpcpy(mempcpy(dst + strlen(dst), src, strnlen(src, count)), "");
    return dst;
}
