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

typedef __INT8_TYPE__                int8_t;
typedef __UINT8_TYPE__              uint8_t;
typedef __INT16_TYPE__              int16_t;
typedef __UINT16_TYPE__            uint16_t;
typedef __INT32_TYPE__              int32_t;
typedef __UINT32_TYPE__            uint32_t;
typedef __INT64_TYPE__              int64_t;
typedef __UINT64_TYPE__            uint64_t;

typedef __INT_FAST8_TYPE__      int_fast8_t;
typedef __UINT_FAST8_TYPE__    uint_fast8_t;
typedef __INT_FAST16_TYPE__    int_fast16_t;
typedef __UINT_FAST16_TYPE__  uint_fast16_t;
typedef __INT_FAST32_TYPE__    int_fast32_t;
typedef __UINT_FAST32_TYPE__  uint_fast32_t;
typedef __INT_FAST64_TYPE__    int_fast64_t;
typedef __UINT_FAST64_TYPE__  uint_fast64_t;


typedef __INT_LEAST8_TYPE__     int_least8_t;
typedef __UINT_LEAST8_TYPE__   uint_least8_t;
typedef __INT_LEAST16_TYPE__   int_least16_t;
typedef __UINT_LEAST16_TYPE__ uint_least16_t;
typedef __INT_LEAST32_TYPE__   int_least32_t;
typedef __UINT_LEAST32_TYPE__ uint_least32_t;
typedef __INT_LEAST64_TYPE__   int_least64_t;
typedef __UINT_LEAST64_TYPE__ uint_least64_t;

typedef  __INTPTR_TYPE__            intptr_t;
typedef __UINTPTR_TYPE__           uintptr_t;

typedef  __INTMAX_TYPE__            intmax_t;
typedef __UINTMAX_TYPE__           uintmax_t;

#define INT8_MAX            __INT8_MAX__
#define INT16_MAX           __INT16_MAX__
#define INT32_MAX           __INT32_MAX__
#define INT64_MAX           __INT64_MAX__

#define INT8_MIN            (-INT8_MAX-1)
#define INT16_MIN           (-INT16_MAX-1)
#define INT32_MIN           (-INT32_MAX-1)
#define INT64_MIN           (-INT64_MAX-1)

#define UINT8_MAX           __UINT8_MAX__
#define UINT16_MAX          __UINT16_MAX__
#define UINT32_MAX          __UINT32_MAX__
#define UINT64_MAX          __UINT64_MAX__

#define INT_LEAST8_MAX      __UINT_LEAST8_MAX__
#define INT_LEAST16_MAX     __UINT_LEAST16_MAX__
#define INT_LEAST32_MAX     __UINT_LEAST32_MAX__
#define INT_LEAST64_MAX     __UINT_LEAST64_MAX__

#define INT_LEAST8_MIN      (-INT_LEAST8_MAX-1)
#define INT_LEAST16_MIN     (-INT_LEAST2_MAX-1)
#define INT_LEAST32_MIN     (-INT_LEAST4_MAX-1)
#define INT_LEAST64_MIN     (-INT_LEAST6_MAX-1)

#define UINT_LEAST8_MAX     __UINT_LEAST8_MAX__
#define UINT_LEAST16_MAX    __UINT_LEAST16_MAX__
#define UINT_LEAST32_MAX    __UINT_LEAST32_MAX__
#define UINT_LEAST64_MAX    __UINT_LEAST64_MAX__

#define INT_FAST8_MAX       __INT_FAST8_MAX__
#define INT_FAST16_MAX      __INT_FAST16_MAX__
#define INT_FAST32_MAX      __INT_FAST32_MAX__
#define INT_FAST64_MAX      __INT_FAST64_MAX__

#define INT_FAST8_MIN       (-INT_FAST8_MAX-1)
#define INT_FAST16_MIN      (-INT_FAST16_MAX-1)
#define INT_FAST32_MIN      (-INT_FAST32_MAX-1)
#define INT_FAST64_MIN      (-INT_FAST64_MAX-1)

#define UINT_FAST8_MAX      __UINT_FAST8_MAX__
#define UINT_FAST16_MAX     __UINT_FAST16_MAX__
#define UINT_FAST32_MAX     __UINT_FAST32_MAX__
#define UINT_FAST64_MAX     __UINT_FAST64_MAX__

#define INTPTR_MAX          __INTPTR_MAX__
#define INTPTR_MIN          (-INTPTR_MAX-1)

#define INTMAX_MAX          __INTMAX_MAX__
#define INTMAX_MIN          (-INTMAX_MAX-1)

#define PTRDIFF_MAX         __PTRDIFF_MAX__
#define PTRDIFF_MIN         (-PTRDIFF_MAX-1)

#define UINTPTR_MAX         __UINTPTR_MAX__

#define UINTMAX_MAX         __UINTMAX_MAX__

#define SIZE_MAX            __SIZE_MAX__

// TODO:
// #define SIG_ATOMIC_MIN      0
// #define SIG_ATOMIC_MAX      0
// #define WCHAR_MIN           0
// #define WCHAR_MAX           0
// #define WINT_MIN            0
// #define WINT_MAX            0

#define INT8_C(c)           __INT8_C(c)
#define UINT8_C(c)          __UINT8_C(c)
#define INT16_C(c)          __INT16_C(c)
#define UINT16_C(c)         __UINT16_C(c)
#define INT32_C(c)          __INT32_C(c)
#define UINT32_C(c)         __UINT32_C(c)
#define INT64_C(c)          __INT64_C(c)
#define UINT64_C(c)         __UINT64_C(c)
#define INTMAX_C(c)         __INTMAX_C(c)
#define UINTMAX_C(c)        __UINTMAX_C(c)

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
