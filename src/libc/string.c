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

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char _strerr_buf[32];

char * strerror(int errnum)
{
    switch (errnum) {
        case 0:       return "Success";
        case ENOMEM:  return "Not enough memory";
        case ENFILE:  return "Too many files open in system";
        case EBADF:   return "Bad file descriptor";
        case ENOSYS:  return "Function not implemented";
        case EMFILE:  return "Too many files open in process";
        case ERANGE:  return "Result too large";
        case ENODEV:  return "No such device";
        case EPERM:   return "Operation not permitted";
        case EFAULT:  return "Bad address";
        case EINVAL:  return "Invalid argument";
        case ENXIO:   return "No such device or address";
        case ENOTTY:  return "Invalid I/O control operation";
        case EAGAIN:  return "Resource unavailable, try again";
        case ENOENT:  return "No such file or directory";
        case EBUSY:   return "Device or resource busy";
        case EBADRQC: return "Invalid request descriptor";
        case EIO:     return "Input/output error";
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

static unsigned long long _strtoull(
    const char *restrict str, char **restrict str_end, int base)
{
    if ((base < 2 || base > 36) && base != 0) {
        errno = EINVAL; // not part of the spec but reasonable
        return 0;
    }
    if (str_end) {
        *str_end = (char *) str;
    }

    int digit;
    int sign = 1;
    int length = 0;
    unsigned long long value = 0;
    // signed long long signed_value = 0;
    bool valid = false;
    // bool overflow = false;
    bool sign_seen = false;
    bool zero_seen = false;
    const char *p = str;

    enum {
        S_SPACE,
        S_SIGN,
        S_PREFIX,
        S_NUMBER,
    };
    int state = S_SPACE;

    do {
        if (state == S_SPACE) {
            if (isspace(*p)) {
                continue;
            }
            state = S_SIGN;
        }

        if (state == S_SIGN) {
            if (!sign_seen && *p == '+') {
                sign_seen = true;
                sign = 1;
                continue;
            }
            if (!sign_seen && *p == '-') {
                sign_seen = true;
                sign = -1;
                continue;
            }
            state = S_PREFIX;
        }

        if (state == S_PREFIX) {
            if (base == 0 || base == 8 || base == 16) {
                if (!zero_seen && *p == '0') {
                    zero_seen = true;
                    base = 8;
                    valid = true;
                    continue;
                }
                if (zero_seen && tolower(*p) == 'x') {
                    base = 16;
                    continue;
                }
                if (base == 0) {
                    base = 10;
                }
            }
            state = S_NUMBER;
        }

        if (state == S_NUMBER) {
            if (!isalnum(*p)) {
                p++;
                break;
            }

            if (isdigit(*p)) {
                digit = (*p - '0');
            }
            else {
                digit = 10 + (tolower(*p) - 'a');
            }

            if (digit >= base) {
                p++;
                break;
            }


            // overflow |= __builtin_mul_overflow_p(value, base, (signed long long) 0);
            // overflow |= __builtin_add_overflow_p(value, digit, (signed long long) 0);
            // if (overflow) {
            //     printf("!!! signed overflow\n");
            //     break;
            // }

            // overflow |= __builtin_mul_overflow_p(value, base, (unsigned long long) 0);
            // overflow |= __builtin_add_overflow_p(value, digit, (unsigned long long) 0);
            // if (overflow) {
            //     printf("!!! unsigned overflow\n");
            //     overflow = false;
            // }

            value *= base;
            value += digit;
            valid = true;
            length++;
        }
    } while (*p++ != '\0');

    value *= sign;

    if (length == 0 && base == 16 && zero_seen) {
        // edge case: base=0 and input is '0x',
        // str_end should point to the x, not the NUL
        p--;
    }

    if (str_end) {
        if (valid) {
            *str_end = (char *) (p - 1);
        }
        else {
            *str_end = (char *) str;
        }
    }
    return value;
}

// TODO: need bounds checking for these!!!

long strtol(const char *restrict str, char **restrict str_end, int base)
{
    return (long) _strtoull(str, str_end, base);
}

unsigned long strtoul(const char *restrict str, char **restrict str_end, int base)
{
    return (unsigned long) _strtoull(str, str_end, base);
}

long long strtoll(const char *restrict str, char **restrict str_end, int base)
{
    return (long long) _strtoull(str, str_end, base);
}

unsigned long long strtoull(const char *restrict str, char **restrict str_end, int base)
{
    return (unsigned long long) _strtoull(str, str_end, base);
}
