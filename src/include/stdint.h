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
 *         File: include/stdint.h
 *      Created: December 29, 2023
 *       Author: Wes Hampson
 *
 * Fixed-width integer types.
 *
 * https://en.cppreference.com/w/c/types/integer (C11)
 * =============================================================================
 */

#ifndef __STDINT_H
#define __STDINT_H

typedef   signed char            int8_t;
typedef unsigned char           uint8_t;
typedef   signed short          int16_t;
typedef unsigned short         uint16_t;
typedef   signed long           int32_t;
typedef unsigned long          uint32_t;
typedef   signed long long      int64_t;
typedef unsigned long long     uint64_t;

typedef   int8_t            int_fast8_t;
typedef  uint8_t           uint_fast8_t;
typedef  int32_t           int_fast16_t;
typedef uint32_t          uint_fast16_t;
typedef  int32_t           int_fast32_t;
typedef uint32_t          uint_fast32_t;
typedef  int64_t           int_fast64_t;
typedef uint64_t          uint_fast64_t;

typedef   int8_t           int_least8_t;
typedef  uint8_t          uint_least8_t;
typedef  int16_t          int_least16_t;
typedef uint16_t         uint_least16_t;
typedef  int32_t          int_least32_t;
typedef uint32_t         uint_least32_t;
typedef  int64_t          int_least64_t;
typedef uint64_t         uint_least64_t;

typedef  int32_t               intptr_t;
typedef uint32_t              uintptr_t;

typedef  int64_t               intmax_t;
typedef uint64_t              uintmax_t;

#define INT8_MAX            (127)
#define INT16_MAX           (32767)
#define INT32_MAX           (2147483647)
#define INT64_MAX           (9223372036854775807LL)

#define INT8_MIN            (-INT8_MAX-1)
#define INT32_MIN           (-INT32_MAX-1)
#define INT64_MIN           (-INT64_MAX-1)
#define INT16_MIN           (-INT16_MAX-1)

#define UINT8_MAX           (255)
#define UINT16_MAX          (65535)
#define UINT32_MAX          (4294967295U)
#define UINT64_MAX          (18446744073709551615ULL)

#define INT_LEAST8_MAX      INT8_MAX
#define INT_LEAST16_MAX     INT16_MAX
#define INT_LEAST32_MAX     INT32_MAX
#define INT_LEAST64_MAX     INT64_MAX

#define INT_LEAST8_MIN      INT8_MIN
#define INT_LEAST16_MIN     INT16_MIN
#define INT_LEAST32_MIN     INT32_MIN
#define INT_LEAST64_MIN     INT64_MIN

#define UINT_LEAST8_MAX     UINT8_MAX
#define UINT_LEAST16_MAX    UINT16_MAX
#define UINT_LEAST32_MAX    UINT32_MAX
#define UINT_LEAST64_MAX    UINT64_MAX

#define INT_FAST8_MAX       INT32_MAX
#define INT_FAST16_MAX      INT32_MAX
#define INT_FAST32_MAX      INT32_MAX
#define INT_FAST64_MAX      INT64_MAX

#define INT_FAST8_MIN       INT32_MIN
#define INT_FAST16_MIN      INT32_MIN
#define INT_FAST32_MIN      INT32_MIN
#define INT_FAST64_MIN      INT64_MIN

#define UINT_FAST8_MAX      UINT32_MAX
#define UINT_FAST16_MAX     UINT32_MAX
#define UINT_FAST32_MAX     UINT32_MAX
#define UINT_FAST64_MAX     UINT64_MAX

#define INTPTR_MIN          INT32_MIN
#define INTPTR_MAX          INT32_MAX

#define INTMAX_MIN          INT64_MIN
#define INTMAX_MAX          INT64_MAX

#define PTRDIFF_MIN         INT32_MIN
#define PTRDIFF_MAX         INT32_MAX

#define UINTPTR_MAX         UINT32_MAX
#define UINTMAX_MAX         UINT64_MAX
#define SIZE_MAX            UINT32_MAX

// TODO:
// #define SIG_ATOMIC_MIN      0
// #define SIG_ATOMIC_MAX      0
// #define WCHAR_MIN           0
// #define WCHAR_MAX           0
// #define WINT_MIN            0
// #define WINT_MAX            0

#define INT8_C(c)           (c)
#define UINT8_C(c)          (c)
#define INT16_C(c)          (c)
#define UINT16_C(c)         (c)
#define INT32_C(c)          (c)
#define UINT32_C(c)         (c ## U)
#define INT64_C(c)          (c ## LL)
#define UINT64_C(c)         (c ## ULL)
#define INTMAX_C(c)         INT64_C(c)
#define UINTMAX_C(c)        UINT64_C(c)

_Static_assert(        sizeof(int8_t) == 1,     "sizeof(int8_t) == 1");
_Static_assert(       sizeof(uint8_t) == 1,     "sizeof(uint8_t) == 1");
_Static_assert(       sizeof(int16_t) == 2,     "sizeof(int16_t) == 2");
_Static_assert(      sizeof(uint16_t) == 2,     "sizeof(uint16_t) == 2");
_Static_assert(       sizeof(int32_t) == 4,     "sizeof(int32_t) == 4");
_Static_assert(      sizeof(uint32_t) == 4,     "sizeof(uint32_t) == 4");
_Static_assert(       sizeof(int64_t) == 8,     "sizeof(int64_t) == 8");
_Static_assert(      sizeof(uint64_t) == 8,     "sizeof(uint64_t) == 8");

_Static_assert(   sizeof(int_fast8_t) >= 1,     "sizeof(int_fast8_t) >= 1");
_Static_assert(  sizeof(uint_fast8_t) >= 1,     "sizeof(uint_fast8_t) >= 1");
_Static_assert(  sizeof(int_fast16_t) >= 2,     "sizeof(int_fast16_t) >= 2");
_Static_assert( sizeof(uint_fast16_t) >= 2,     "sizeof(uint_fast16_t) >= 2");
_Static_assert(  sizeof(int_fast32_t) >= 4,     "sizeof(int_fast32_t) >= 4");
_Static_assert( sizeof(uint_fast32_t) >= 4,     "sizeof(uint_fast32_t) >= 4");
_Static_assert(  sizeof(int_fast64_t) >= 8,     "sizeof(int_fast64_t) >= 8");
_Static_assert( sizeof(uint_fast64_t) >= 8,     "sizeof(uint_fast64_t) >= 8");

_Static_assert(  sizeof(int_least8_t) >= 1,     "sizeof(int_least8_t) >= 1");
_Static_assert( sizeof(uint_least8_t) >= 1,     "sizeof(uint_least8_t) >= 1");
_Static_assert( sizeof(int_least16_t) >= 2,     "sizeof(int_least16_t) >= 2");
_Static_assert(sizeof(uint_least16_t) >= 2,     "sizeof(uint_least16_t) >= 2");
_Static_assert( sizeof(int_least32_t) >= 4,     "sizeof(int_least32_t) >= 4");
_Static_assert(sizeof(uint_least32_t) >= 4,     "sizeof(uint_least32_t) >= 4");
_Static_assert( sizeof(int_least64_t) >= 8,     "sizeof(int_least64_t) >= 8");
_Static_assert(sizeof(uint_least64_t) >= 8,     "sizeof(uint_least64_t) >= 8");

#endif /* __STDINT_H */
