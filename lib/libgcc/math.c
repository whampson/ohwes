/* =============================================================================
 * Copyright (C) 2020-2024 Wes Hampson. All Rights Reserved.
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
 *         File: lib/libgcc/math.c
 *      Created: December 26, 2023
 *       Author: Wes Hampson
 *
 * GCC math routines for operations that don't have hardware support.
 *
 * https://gcc.gnu.org/onlinedocs/gccint/Integer-library-routines.html
 * =============================================================================
 */

#ifdef __GNUC__

#include <stddef.h>
#include <stdint.h>

/**
 * https://dox.ipxe.org/____udivmoddi4_8c.html
 */
uint64_t __udivmoddi4(uint64_t num, uint64_t den, uint64_t *rem_p)
{
   uint64_t quot = 0, qbit = 1;

    if (den == 0) {
        /* Intentional divide by zero, without
           triggering a compiler warning which
           would abort the build */
        return 1 / ((unsigned) den);
    }

    /* Left-justify denominator and count shift */
    while ((int64_t) den >= 0) {
        den <<= 1;
        qbit <<= 1;
    }

    while (qbit) {
        if (den <= num) {
            num -= den;
            quot += qbit;
        }
        den >>= 1;
        qbit >>= 1;
    }

    if (rem_p) {
        *rem_p = num;
    }

    return quot;
}

/**
 * https://dox.ipxe.org/____divmoddi4_8c.html
 */
int64_t __divmoddi4(int64_t num, int64_t den, int64_t *rem_p)
{
    int minus = 0;
    int64_t v = 0;

    if (num < 0) {
        num = -num;
        minus = 1;
    }
    if (den < 0) {
        den = -den;
        minus ^= 1;
    }

    v = __udivmoddi4(num, den, (uint64_t *) rem_p);
    if (minus) {
        v = -v;
        if (rem_p) {
            *rem_p = -(*rem_p);
        }
    }

    return v;
}

int64_t __divdi3(int64_t num, int64_t den)
{
    return __divmoddi4(num, den, NULL);
}

int64_t __moddi3(int64_t num, int64_t den)
{
    int64_t v = 0;
    (void) __divmoddi4(num, den, &v);
    return v;
}

uint64_t __udivdi3(uint64_t num, uint64_t den)
{
    return __udivmoddi4(num, den, NULL);
}

uint64_t __umoddi3(uint64_t num, uint64_t den)
{
    uint64_t v = 0;
    (void) __udivmoddi4(num, den, &v);
    return v;
}

#endif  /* __GNUC__ */