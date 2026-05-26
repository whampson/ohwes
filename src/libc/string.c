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
        case 0:         return "Success";
        case ENOMEM:    return "Not enough memory";
        case ENFILE:    return "Too many files open in system";
        case EBADF:     return "Bad file descriptor";
        case ENOSYS:    return "Function not implemented";
        case EMFILE:    return "Too many files open in process";
        case ERANGE:    return "Result too large";
        case ENODEV:    return "No such device";
        case EPERM:     return "Operation not permitted";
        case EFAULT:    return "Bad address";
        case EINVAL:    return "Invalid argument";
        case ENXIO:     return "No such device or address";
        case ENOTTY:    return "Invalid I/O control operation";
        case EAGAIN:    return "Resource unavailable, try again";
        case ENOENT:    return "No such file or directory";
        case EBUSY:     return "Device or resource busy";
        case EBADRQC:   return "Invalid request descriptor";
        case EIO:       return "Input/output error";
        case EDOM:      return "Math argument out of function domain";
        case EILSEQ:    return "Illegal byte sequence";
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
    const unsigned char *l = lhs;
    const unsigned char *r = rhs;

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

    return (unsigned char) *lhs - (unsigned char) *rhs;
}

int strncmp(const char *lhs, const char *rhs, size_t count)
{
    if (count == 0) {
        return 0;
    }

    int diff = 0;
    for (int i = 0; i < count; i++, lhs++, rhs++) {
        diff = (unsigned char) *lhs - (unsigned char) *rhs;
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

enum strto_type {
    STRTO_L,
    STRTO_LL,
    STRTO_UL,
    STRTO_ULL,
};

static uint64_t _strtol_impl(enum strto_type type,
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
    int length = 0;
    uint64_t value = 0;
    bool sign = false;
    bool overflow = false;
    bool range_error = false;
    bool digits_seen = false;
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
                continue;
            }
            if (!sign_seen && *p == '-') {
                sign_seen = true;
                sign = true;
                continue;
            }
            state = S_PREFIX;
        }

        if (state == S_PREFIX) {
            if (base == 0 || base == 8 || base == 16) {
                if (!zero_seen && *p == '0') {
                    digits_seen = true;
                    zero_seen = true;
                    base = 8;
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
                if (digits_seen) {
                    range_error = true;
                }
                p++;
                break;
            }

            #define OVERFLOW_CHECK(t) \
            do { \
                overflow |= __builtin_mul_overflow_p(value, base,  (t) 0); \
                overflow |= __builtin_add_overflow_p(value*base, digit, (t) 0); \
            } while (0)

            switch (type) {
                case STRTO_L:   OVERFLOW_CHECK(signed long);        break;
                case STRTO_LL:  OVERFLOW_CHECK(signed long long);   break;
                case STRTO_UL:  OVERFLOW_CHECK(unsigned long);      break;
                case STRTO_ULL: OVERFLOW_CHECK(unsigned long long); break;
            }

            digits_seen = true;
            value *= base;
            value += digit;
            length++;
        }
    } while (*p++ != '\0');

    if (sign) {
        value *= -1;
    }

    if (length == 0 && base == 16 && zero_seen) {
        // edge case: base=0 and input is '0x',
        // str_end should point to the x, not the NUL
        p--;
    }

    if (str_end && digits_seen) {
        *str_end = (char *) (p - 1);
    }

    if (overflow) {
        range_error |= !(type == STRTO_L  && sign && (value == LONG_MIN)) &&
                       !(type == STRTO_LL && sign && (value == LLONG_MIN));
        value = (type == STRTO_L)   ? ((sign) ? LONG_MIN  : LONG_MAX)  :
                (type == STRTO_LL)  ? ((sign) ? LLONG_MIN : LLONG_MAX) :
                (type == STRTO_UL)  ? ULONG_MAX  :
                (type == STRTO_ULL) ? ULLONG_MAX :
                value;
    }

    if (range_error) {
        errno = ERANGE;
    }

    return value;
}

#define DEFINE_STRTO(t_char,t_enum,t) \
t strto##t_char(const char *restrict str, char **restrict str_end, int base) \
{ \
    return (t) _strtol_impl(t_enum, str, str_end, base); \
}

DEFINE_STRTO(l,   STRTO_L,   long)
DEFINE_STRTO(ll,  STRTO_LL,  long long)
DEFINE_STRTO(ul,  STRTO_UL,  unsigned long)
DEFINE_STRTO(ull, STRTO_ULL, unsigned long long)

#undef DEFINE_STRTO

char * strtok_r(char *restrict str, const char *restrict delim, char **restrict saveptr)
{
    if (delim == NULL || saveptr == NULL) {
        return NULL;
    }
    if (str != NULL) {
        *saveptr = str;
    }

    for (; **saveptr != '\0'; (*saveptr)++) {
        bool got_delim = false;
        for (int i = 0; delim[i] != '\0'; i++) {
            if (**saveptr == delim[i]) {
                got_delim = true;
                break;
            }
        }
        if (!got_delim) {
            break;
        }
    }

    if (**saveptr == '\0') {
        return NULL;
    }

    char *tok = *saveptr;
    for (; **saveptr != '\0'; (*saveptr)++) {
        for (int i = 0; delim[i] != '\0'; i++) {
            if (**saveptr == delim[i]) {
                *(*saveptr)++ = '\0';
                return tok;
            }
        }
    }

    return tok;
}

char * strtok(char *restrict str, const char *restrict delim)
{
    static char *_str = NULL;
    return strtok_r(str, delim, &_str);
}
