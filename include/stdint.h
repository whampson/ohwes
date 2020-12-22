/*============================================================================*
 * Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                  *
 *                                                                            *
 * This file is part of the Niobium Operating System.                         *
 * Niobium is free software; you may redistribute it and/or modify it under   *
 * the terms of the license agreement provided with this software.            *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: include/stdint.h                                                  *
 * Created: December 11, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *                                                                            *
 * Implementation of stdint.h from the C Standard Library.                    *
 *============================================================================*/

/* Completion Status: INCOMPLETE */

/**
 * TODO:
 * SIG_ATOMIC_MIN
 * SIG_ATOMIC_MAX
 * WCHAR_MIN
 * WCHAR_MAX
 * WINT_MIN
 * WINT_MAX
 */

#ifndef __STDINT_H
#define __STDINT_H

typedef signed char                     int8_t;
typedef signed short int                int16_t;
typedef signed long int                 int32_t;
typedef signed long long int            int64_t;
typedef signed long int                 int_fast8_t;
typedef signed long int                 int_fast16_t;
typedef signed long int                 int_fast32_t;
typedef signed long long int            int_fast64_t;
typedef signed char                     int_least8_t;
typedef signed short int                int_least_16_t;
typedef signed long int                 int_least_32_t;
typedef signed long long int            int_least_64_t;
typedef int64_t                         intmax_t;
typedef int32_t                         intptr_t;

typedef unsigned char                   uint8_t;
typedef unsigned short int              uint16_t;
typedef unsigned long int               uint32_t;
typedef unsigned long long int          uint64_t;
typedef unsigned long int               uint_fast8_t;
typedef unsigned long int               uint_fast16_t;
typedef unsigned long int               uint_fast32_t;
typedef unsigned long long int          uint_fast64_t;
typedef unsigned char                   uint_least8_t;
typedef unsigned short int              uint_least_16_t;
typedef unsigned long int               uint_least_32_t;
typedef unsigned long long int          uint_least_64_t;
typedef uint64_t                        uintmax_t;
typedef uint32_t                        uintptr_t;

#define INT8_MIN                        (-0x7F-1)
#define INT16_MIN                       (-0x7FFF-1)
#define INT32_MIN                       (-0x7FFFFFFF-1)
#define INT64_MIN                       (-0x7FFFFFFFFFFFFFFFLL-1)
#define INT_FAST8_MIN                   INT32_MIN
#define INT_FAST16_MIN                  INT32_MIN
#define INT_FAST32_MIN                  INT32_MIN
#define INT_FAST64_MIN                  INT64_MIN
#define INT_LEAST8_MIN                  INT8_MIN
#define INT_LEAST16_MIN                 INT16_MIN
#define INT_LEAST32_MIN                 INT32_MIN
#define INT_LEAST64_MIN                 INT64_MIN
#define INTPTR_MIN                      INT32_MIN
#define INTMAX_MIN                      INT64_MIN
#define PTRDIFF_MIN                     INT32_MIN

#define INT8_MAX                        0x7F
#define INT16_MAX                       0x7FFF
#define INT32_MAX                       0x7FFFFFFF
#define INT64_MAX                       0x7FFFFFFFFFFFFFFFLL
#define INT_FAST8_MAX                   INT32_MAX
#define INT_FAST16_MAX                  INT32_MAX
#define INT_FAST32_MAX                  INT32_MAX
#define INT_FAST64_MAX                  INT64_MAX
#define INT_LEAST8_MAX                  INT8_MAX
#define INT_LEAST16_MAX                 INT16_MAX
#define INT_LEAST32_MAX                 INT32_MAX
#define INT_LEAST64_MAX                 INT64_MIN
#define INTPTR_MAX                      INT32_MAX
#define INTMAX_MAX                      INT64_MAX
#define PTRDIFF_MAX                     INT32_MAX

#define UINT8_MAX                       0xFF
#define UINT16_MAX                      0xFFFF
#define UINT32_MAX                      0xFFFFFFFF
#define UINT64_MAX                      0xFFFFFFFFFFFFFFFFULL
#define UINT_FAST8_MAX                  UINT32_MAX
#define UINT_FAST16_MAX                 UINT32_MAX
#define UINT_FAST32_MAX                 UINT32_MAX
#define UINT_FAST64_MAX                 UINT64_MAX
#define UINT_LEAST8_MAX                 UINT8_MAX
#define UINT_LEAST16_MAX                UINT16_MAX
#define UINT_LEAST32_MAX                UINT32_MAX
#define UINT_LEAST64_MAX                UINT64_MAX
#define UINTPTR_MAX                     UINT32_MAX
#define UINTMAX_MAX                     UINT64_MAX
#define SIZE_MAX                        UINT32_MAX

#define INT8_C(c)                       (c)
#define INT16_C(c)                      (c)
#define INT32_C(c)                      (c)
#define INT64_C(c)                      (c ## LL)
#define INTMAX_C(c)                     INT64_C(c)

#define UINT8_C(c)                      (c)
#define UINT16_C(c)                     (c)
#define UINT32_C(c)                     (c ## U)
#define UINT64_C(c)                     (c ## ULL)
#define UINTMAX_C(c)                    UINT64_C(c)

_Static_assert(sizeof(int8_t)           == 1, "sizeof(int8_t)");
_Static_assert(sizeof(int16_t)          == 2, "sizeof(int16_t)");
_Static_assert(sizeof(int32_t)          == 4, "sizeof(int32_t)");
_Static_assert(sizeof(int64_t)          == 8, "sizeof(int64_t)");
_Static_assert(sizeof(int_least8_t)     >= 1, "sizeof(int_least8_t)");
_Static_assert(sizeof(int_least_16_t)   >= 2, "sizeof(int_least_16_t)");
_Static_assert(sizeof(int_least_32_t)   >= 4, "sizeof(int_least_32_t)");
_Static_assert(sizeof(int_least_64_t)   >= 8, "sizeof(int_least_64_t)");
_Static_assert(sizeof(uint8_t)          == 1, "sizeof(uint8_t)");
_Static_assert(sizeof(uint16_t)         == 2, "sizeof(uint16_t)");
_Static_assert(sizeof(uint32_t)         == 4, "sizeof(uint32_t)");
_Static_assert(sizeof(uint64_t)         == 8, "sizeof(uint64_t)");
_Static_assert(sizeof(uint_least8_t)    >= 1, "sizeof(uint_least8_t)");
_Static_assert(sizeof(uint_least_16_t)  >= 2, "sizeof(uint_least_16_t)");
_Static_assert(sizeof(uint_least_32_t)  >= 4, "sizeof(uint_least_32_t)");
_Static_assert(sizeof(uint_least_64_t)  >= 8, "sizeof(uint_least_64_t)");

#endif /* __STDINT_H */
