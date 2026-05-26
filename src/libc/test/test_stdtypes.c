/* =============================================================================
 * Copyright (C) 2020-2026 Wes Hampson. All Rights Reserved.
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
 *         File: src/libc/test/test_stdtypes.c
 *      Created: May 24, 2026
 *       Author: Wes Hampson
 *
 * libc type and constant header tests
 *
 * Tests for: stdbool.h, stddef.h, stdint.h, errno.h, inttypes.h, limits.h
 *
 * Covers: type sizes, MIN/MAX constants, format macros, offsetof,
 *         errno behavior, bool semantics, fixed-width type correctness.
 * =============================================================================
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include "framework.h"

/* ========================================================================= */
/*  stdbool.h                                                                */
/* ========================================================================= */


static void test_bool_type(void)
{
    TEST("bool: _Bool holds 0 and 1");
    _Bool a = 1;
    _Bool b = 0;
    ASSERT_EQ_INT(1, a, "_Bool 1");
    ASSERT_EQ_INT(0, b, "_Bool 0");
    PASS();
}

static void test_bool_conversion(void)
{
    TEST("bool: non-zero converts to true");
    _Bool a = 42;
    _Bool b = -1;
    ASSERT_EQ_INT(1, a, "42 -> true");
    ASSERT_EQ_INT(1, b, "-1 -> true");
    PASS();
}

/* ========================================================================= */
/*  stddef.h                                                                 */
/* ========================================================================= */


static void test_null_pointer(void)
{
    TEST("stddef: NULL pointer comparison");
    void *p = NULL;
    ASSERT(p == NULL, "pointer is NULL");
    ASSERT(!p, "NULL is falsy");
    PASS();
}

static void test_size_t_size(void)
{
    TEST("stddef: size_t is pointer-sized");
    ASSERT(sizeof(size_t) == sizeof(void *), "size_t == sizeof(void*)");
    PASS();
}

static void test_ptrdiff_t_size(void)
{
    TEST("stddef: ptrdiff_t is pointer-sized");
    ASSERT(sizeof(ptrdiff_t) == sizeof(void *),
           "ptrdiff_t == sizeof(void*)");
    PASS();
}

static void test_offsetof_basic(void)
{
    TEST("stddef: offsetof first member == 0");
    struct S { int a; int b; char c; };
    ASSERT_EQ_INT(0, (int)offsetof(struct S, a), "first member at 0");
    PASS();
}

static void test_offsetof_later_member(void)
{
    TEST("stddef: offsetof later member > 0");
    struct S { int a; int b; char c; };
    ASSERT(offsetof(struct S, b) >= sizeof(int), "b after a");
    ASSERT(offsetof(struct S, c) > offsetof(struct S, b), "c after b");
    PASS();
}

static void test_offsetof_char_packed(void)
{
    TEST("stddef: offsetof with char members");
    struct S { char a; char b; char c; };
    ASSERT_EQ_INT(0, (int)offsetof(struct S, a), "a at 0");
    ASSERT_EQ_INT(1, (int)offsetof(struct S, b), "b at 1");
    ASSERT_EQ_INT(2, (int)offsetof(struct S, c), "c at 2");
    PASS();
}

static void test_offsetof_with_padding(void)
{
    TEST("stddef: offsetof accounts for alignment padding");
    struct Padded { char a; int b; };
    ASSERT(offsetof(struct Padded, b) >= 2, "b after padding");
    ASSERT(offsetof(struct Padded, b) == sizeof(int) ||
           offsetof(struct Padded, b) >= 2, "aligned offset");
    PASS();
}

/* ========================================================================= */
/*  stdint.h: type sizes                                                     */
/* ========================================================================= */

static void test_int8_size(void)
{
    TEST("stdint: sizeof(int8_t) == 1");
    ASSERT_EQ_INT(1, (int)sizeof(int8_t), "int8_t is 1 byte");
    ASSERT_EQ_INT(1, (int)sizeof(uint8_t), "uint8_t is 1 byte");
    PASS();
}

static void test_int16_size(void)
{
    TEST("stdint: sizeof(int16_t) == 2");
    ASSERT_EQ_INT(2, (int)sizeof(int16_t), "int16_t is 2 bytes");
    ASSERT_EQ_INT(2, (int)sizeof(uint16_t), "uint16_t is 2 bytes");
    PASS();
}

static void test_int32_size(void)
{
    TEST("stdint: sizeof(int32_t) == 4");
    ASSERT_EQ_INT(4, (int)sizeof(int32_t), "int32_t is 4 bytes");
    ASSERT_EQ_INT(4, (int)sizeof(uint32_t), "uint32_t is 4 bytes");
    PASS();
}

static void test_int64_size(void)
{
    TEST("stdint: sizeof(int64_t) == 8");
    ASSERT_EQ_INT(8, (int)sizeof(int64_t), "int64_t is 8 bytes");
    ASSERT_EQ_INT(8, (int)sizeof(uint64_t), "uint64_t is 8 bytes");
    PASS();
}

static void test_intptr_size(void)
{
    TEST("stdint: intptr_t is pointer-sized");
    ASSERT(sizeof(intptr_t) == sizeof(void *), "intptr_t");
    ASSERT(sizeof(uintptr_t) == sizeof(void *), "uintptr_t");
    PASS();
}

static void test_intmax_size(void)
{
    TEST("stdint: intmax_t >= 8 bytes");
    ASSERT(sizeof(intmax_t) >= 8, "intmax_t at least 64-bit");
    ASSERT(sizeof(uintmax_t) >= 8, "uintmax_t at least 64-bit");
    PASS();
}

/* ========================================================================= */
/*  stdint.h: MIN/MAX constants                                              */
/* ========================================================================= */

static void test_int8_limits(void)
{
    TEST("stdint: INT8_MIN/MAX correct");
    ASSERT_EQ_INT(-128, INT8_MIN, "INT8_MIN");
    ASSERT_EQ_INT(127, INT8_MAX, "INT8_MAX");
    ASSERT_EQ_INT(255, (int)UINT8_MAX, "UINT8_MAX");
    PASS();
}

static void test_int16_limits(void)
{
    TEST("stdint: INT16_MIN/MAX correct");
    ASSERT_EQ_INT(-32768, INT16_MIN, "INT16_MIN");
    ASSERT_EQ_INT(32767, INT16_MAX, "INT16_MAX");
    ASSERT_EQ_INT(65535, (int)UINT16_MAX, "UINT16_MAX");
    PASS();
}

static void test_int32_limits(void)
{
    TEST("stdint: INT32_MIN/MAX correct");
    ASSERT(INT32_MIN == (-2147483647 - 1), "INT32_MIN");
    ASSERT(INT32_MAX == 2147483647, "INT32_MAX");
    ASSERT(UINT32_MAX == 4294967295U, "UINT32_MAX");
    PASS();
}

static void test_int64_limits(void)
{
    TEST("stdint: INT64_MIN/MAX correct");
    ASSERT(INT64_MIN == (-9223372036854775807LL - 1), "INT64_MIN");
    ASSERT(INT64_MAX == 9223372036854775807LL, "INT64_MAX");
    ASSERT(UINT64_MAX == 18446744073709551615ULL, "UINT64_MAX");
    PASS();
}

static void test_size_max(void)
{
    TEST("stdint: SIZE_MAX matches pointer size");
    if (sizeof(size_t) == 4)
        ASSERT(SIZE_MAX == 4294967295U, "32-bit SIZE_MAX");
    else
        ASSERT(SIZE_MAX == 18446744073709551615ULL, "64-bit SIZE_MAX");
    PASS();
}

/* ========================================================================= */
/*  errno.h                                                                  */
/* ========================================================================= */

static void test_errno_modifiable(void)
{
    TEST("errno: is modifiable");
    errno = 0;
    ASSERT_EQ_INT(0, errno, "set to 0");
    errno = EDOM;
    ASSERT_EQ_INT(EDOM, errno, "set to EDOM");
    errno = 0;
    PASS();
}

static void test_errno_edom_exists(void)
{
    TEST("errno: EDOM is defined and nonzero");
    ASSERT(EDOM != 0, "EDOM nonzero");
    PASS();
}

static void test_errno_erange_exists(void)
{
    TEST("errno: ERANGE is defined and nonzero");
    ASSERT(ERANGE != 0, "ERANGE nonzero");
    PASS();
}

static void test_errno_eilseq_exists(void)
{
    TEST("errno: EILSEQ is defined and nonzero");
    ASSERT(EILSEQ != 0, "EILSEQ nonzero");
    PASS();
}

static void test_errno_constants_distinct(void)
{
    TEST("errno: EDOM, ERANGE, EILSEQ are distinct");
    ASSERT(EDOM != ERANGE, "EDOM != ERANGE");
    ASSERT(EDOM != EILSEQ, "EDOM != EILSEQ");
    ASSERT(ERANGE != EILSEQ, "ERANGE != EILSEQ");
    PASS();
}

/* ========================================================================= */
/*  limits.h                                                                 */
/* ========================================================================= */

static void test_char_bit(void)
{
    TEST("limits: CHAR_BIT == 8");
    ASSERT_EQ_INT(8, CHAR_BIT, "8 bits per byte");
    PASS();
}

static void test_char_limits(void)
{
    TEST("limits: CHAR_MIN/CHAR_MAX correct");
    if ((char)-1 < 0)
    {
        ASSERT_EQ_INT(SCHAR_MIN, CHAR_MIN, "signed CHAR_MIN");
        ASSERT_EQ_INT(SCHAR_MAX, CHAR_MAX, "signed CHAR_MAX");
    }
    else
    {
        ASSERT_EQ_INT(0, CHAR_MIN, "unsigned CHAR_MIN");
        ASSERT_EQ_INT(UCHAR_MAX, CHAR_MAX, "unsigned CHAR_MAX");
    }
    PASS();
}

static void test_schar_limits(void)
{
    TEST("limits: SCHAR_MIN/MAX correct");
    ASSERT_EQ_INT(-128, SCHAR_MIN, "SCHAR_MIN");
    ASSERT_EQ_INT(127, SCHAR_MAX, "SCHAR_MAX");
    ASSERT_EQ_INT(255, UCHAR_MAX, "UCHAR_MAX");
    PASS();
}

static void test_shrt_limits(void)
{
    TEST("limits: SHRT_MIN/MAX correct");
    ASSERT_EQ_INT(-32768, SHRT_MIN, "SHRT_MIN");
    ASSERT_EQ_INT(32767, SHRT_MAX, "SHRT_MAX");
    ASSERT_EQ_INT(65535, USHRT_MAX, "USHRT_MAX");
    PASS();
}

static void test_int_limits(void)
{
    TEST("limits: INT_MIN/MAX correct");
    ASSERT(INT_MIN == (-2147483647 - 1), "INT_MIN");
    ASSERT(INT_MAX == 2147483647, "INT_MAX");
    ASSERT(UINT_MAX == 4294967295U, "UINT_MAX");
    PASS();
}

static void test_long_limits(void)
{
    TEST("limits: LONG_MIN/MAX correct");
    ASSERT(LONG_MIN < 0, "LONG_MIN negative");
    ASSERT(LONG_MAX > 0, "LONG_MAX positive");
    ASSERT(ULONG_MAX > 0, "ULONG_MAX positive");
    /* Size-dependent: verify consistency */
    if (sizeof(long) == 4)
    {
        ASSERT(LONG_MAX == 2147483647L, "32-bit LONG_MAX");
        ASSERT(ULONG_MAX == 4294967295UL, "32-bit ULONG_MAX");
    }
    else
    {
        ASSERT(LONG_MAX == 9223372036854775807L, "64-bit LONG_MAX");
        ASSERT(ULONG_MAX == 18446744073709551615UL, "64-bit ULONG_MAX");
    }
    PASS();
}

static void test_llong_limits(void)
{
    TEST("limits: LLONG_MIN/MAX correct");
    ASSERT(LLONG_MIN == (-9223372036854775807LL - 1), "LLONG_MIN");
    ASSERT(LLONG_MAX == 9223372036854775807LL, "LLONG_MAX");
    ASSERT(ULLONG_MAX == 18446744073709551615ULL, "ULLONG_MAX");
    PASS();
}

static void test_limits_sizeof_consistency(void)
{
    TEST("limits: sizeof matches limit ranges");
    ASSERT(sizeof(char) == 1, "char is 1 byte");
    ASSERT(sizeof(short) >= 2, "short at least 2 bytes");
    ASSERT(sizeof(int) >= 2, "int at least 2 bytes");
    ASSERT(sizeof(long) >= 4, "long at least 4 bytes");
    ASSERT(sizeof(long long) >= 8, "long long at least 8 bytes");
    PASS();
}

/* ========================================================================= */
/*  inttypes.h: format macros with sprintf                                   */
/* ========================================================================= */

static void test_pri_d_macros(void)
{
    TEST("inttypes: PRId8/16/32/64 produce correct output");
    char buf[32];
    sprintf(buf, "%" PRId8, (int8_t)-1);
    ASSERT_EQ_STR("-1", buf, "PRId8");
    sprintf(buf, "%" PRId16, (int16_t)-1000);
    ASSERT_EQ_STR("-1000", buf, "PRId16");
    sprintf(buf, "%" PRId32, (int32_t)-100000);
    ASSERT_EQ_STR("-100000", buf, "PRId32");
    sprintf(buf, "%" PRId64, (int64_t)-1000000000LL);
    ASSERT_EQ_STR("-1000000000", buf, "PRId64");
    PASS();
}

static void test_pri_u_macros(void)
{
    TEST("inttypes: PRIu8/16/32/64 produce correct output");
    char buf[32];
    sprintf(buf, "%" PRIu8, (uint8_t)255);
    ASSERT_EQ_STR("255", buf, "PRIu8");
    sprintf(buf, "%" PRIu16, (uint16_t)65535);
    ASSERT_EQ_STR("65535", buf, "PRIu16");
    sprintf(buf, "%" PRIu32, (uint32_t)4294967295U);
    ASSERT_EQ_STR("4294967295", buf, "PRIu32");
    sprintf(buf, "%" PRIu64, (uint64_t)18446744073709551615ULL);
    ASSERT_EQ_STR("18446744073709551615", buf, "PRIu64");
    PASS();
}

static void test_pri_x_macros(void)
{
    TEST("inttypes: PRIx32/PRIX64 hex output");
    char buf[32];
    sprintf(buf, "%" PRIx32, (uint32_t)0xDEADBEEF);
    ASSERT_EQ_STR("deadbeef", buf, "PRIx32 lowercase");
    sprintf(buf, "%" PRIX64, (uint64_t)0xCAFEBABEULL);
    ASSERT_EQ_STR("CAFEBABE", buf, "PRIX64 uppercase");
    PASS();
}

static void test_pri_o_macros(void)
{
    TEST("inttypes: PRIo32 octal output");
    char buf[32];
    sprintf(buf, "%" PRIo32, (uint32_t)511);
    ASSERT_EQ_STR("777", buf, "PRIo32");
    PASS();
}

static void test_pri_ptr_macros(void)
{
    TEST("inttypes: PRIdPTR/PRIuPTR work");
    char buf[32];
    intptr_t ip = 12345;
    sprintf(buf, "%" PRIdPTR, ip);
    ASSERT_EQ_STR("12345", buf, "PRIdPTR");
    uintptr_t up = 12345;
    sprintf(buf, "%" PRIuPTR, up);
    ASSERT_EQ_STR("12345", buf, "PRIuPTR");
    PASS();
}

static void test_pri_max_macros(void)
{
    TEST("inttypes: PRIdMAX/PRIuMAX work");
    char buf[32];
    intmax_t im = -999;
    sprintf(buf, "%" PRIdMAX, im);
    ASSERT_EQ_STR("-999", buf, "PRIdMAX");
    uintmax_t um = 999;
    sprintf(buf, "%" PRIuMAX, um);
    ASSERT_EQ_STR("999", buf, "PRIuMAX");
    PASS();
}

/* ========================================================================= */
/*  Suite runner                                                             */
/* ========================================================================= */

TEST_SUITE(stdtypes, "type/constant header tests")
{
    printf(COLOR_YELLOW "[stdbool.h]" COLOR_RESET "\n");
    test_bool_type();
    test_bool_conversion();

    printf(COLOR_YELLOW "[stddef.h]" COLOR_RESET "\n");
    test_null_pointer();
    test_size_t_size();
    test_ptrdiff_t_size();
    test_offsetof_basic();
    test_offsetof_later_member();
    test_offsetof_char_packed();
    test_offsetof_with_padding();

    printf(COLOR_YELLOW "[stdint.h: type sizes]" COLOR_RESET "\n");
    test_int8_size();
    test_int16_size();
    test_int32_size();
    test_int64_size();
    test_intptr_size();
    test_intmax_size();

    printf(COLOR_YELLOW "[stdint.h: MIN/MAX constants]" COLOR_RESET "\n");
    test_int8_limits();
    test_int16_limits();
    test_int32_limits();
    test_int64_limits();
    test_size_max();

    printf(COLOR_YELLOW "[errno.h]" COLOR_RESET "\n");
    test_errno_modifiable();
    test_errno_edom_exists();
    test_errno_erange_exists();
    test_errno_eilseq_exists();
    test_errno_constants_distinct();

    printf(COLOR_YELLOW "[limits.h]" COLOR_RESET "\n");
    test_char_bit();
    test_char_limits();
    test_schar_limits();
    test_shrt_limits();
    test_int_limits();
    test_long_limits();
    test_llong_limits();
    test_limits_sizeof_consistency();

    printf(COLOR_YELLOW "[inttypes.h: format macros]" COLOR_RESET "\n");
    test_pri_d_macros();
    test_pri_u_macros();
    test_pri_x_macros();
    test_pri_o_macros();
    test_pri_ptr_macros();
    test_pri_max_macros();
}
