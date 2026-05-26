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
 *         File: src/libc/test/test_strtol.c
 *      Created: May 24, 2026
 *       Author: Wes Hampson
 *
 * libc strtol-family function tests
 *
 * Covers: basic conversion, bases (2-36, 0 auto-detect), whitespace/sign
 *         handling, endptr behavior, errno/ERANGE overflow/underflow,
 *         boundary values, prefix edge cases, no-conversion cases.
 * =============================================================================
 */

#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <inttypes.h>
#include "framework.h"

/* ========================================================================= */
/*  strtol: basic conversion                                                 */
/* ========================================================================= */

static void test_strtol_positive(void)
{
    TEST("strtol: positive integer");
    char *end;
    long val = strtol("42", &end, 10);
    ASSERT_EQ_INT(42, (int)val, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtol_negative(void)
{
    TEST("strtol: negative integer");
    char *end;
    long val = strtol("-42", &end, 10);
    ASSERT_EQ_INT(-42, (int)val, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtol_zero(void)
{
    TEST("strtol: zero");
    char *end;
    long val = strtol("0", &end, 10);
    ASSERT_EQ_INT(0, (int)val, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtol_explicit_plus(void)
{
    TEST("strtol: explicit + sign");
    char *end;
    long val = strtol("+99", &end, 10);
    ASSERT_EQ_INT(99, (int)val, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtol_null_endptr(void)
{
    TEST("strtol: NULL endptr does not crash");
    long val = strtol("123", NULL, 10);
    ASSERT_EQ_INT(123, (int)val, "value");
    PASS();
}

/* ========================================================================= */
/*  strtol: whitespace handling                                              */
/* ========================================================================= */

static void test_strtol_leading_spaces(void)
{
    TEST("strtol: leading spaces skipped");
    char *end;
    long val = strtol("   42", &end, 10);
    ASSERT_EQ_INT(42, (int)val, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtol_leading_whitespace_mix(void)
{
    TEST("strtol: tabs/newlines/etc skipped");
    char *end;
    long val = strtol(" \t\n\r\v\f123", &end, 10);
    ASSERT_EQ_INT(123, (int)val, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

/* ========================================================================= */
/*  strtol: base handling                                                    */
/* ========================================================================= */

static void test_strtol_base16(void)
{
    TEST("strtol: base 16 without prefix");
    char *end;
    long val = strtol("ff", &end, 16);
    ASSERT_EQ_INT(255, (int)val, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtol_base16_prefix(void)
{
    TEST("strtol: base 16 with 0x prefix");
    char *end;
    long val = strtol("0xff", &end, 16);
    ASSERT_EQ_INT(255, (int)val, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtol_base16_uppercase(void)
{
    TEST("strtol: base 16 uppercase 0XABCDEF");
    char *end;
    long val = strtol("0XABCDEF", &end, 16);
    ASSERT_EQ_INT(0xABCDEF, (int)val, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtol_base8(void)
{
    TEST("strtol: base 8");
    char *end;
    long val = strtol("77", &end, 8);
    ASSERT_EQ_INT(63, (int)val, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtol_base8_prefix(void)
{
    TEST("strtol: base 8 with 0 prefix");
    char *end;
    long val = strtol("077", &end, 8);
    ASSERT_EQ_INT(63, (int)val, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtol_base2(void)
{
    TEST("strtol: base 2 binary");
    char *end;
    long val = strtol("1010", &end, 2);
    ASSERT_EQ_INT(10, (int)val, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtol_base36(void)
{
    TEST("strtol: base 36 max digit z=35");
    char *end;
    long val = strtol("z", &end, 36);
    ASSERT_EQ_INT(35, (int)val, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtol_base36_uppercase(void)
{
    TEST("strtol: base 36 uppercase Z=35");
    char *end;
    long val = strtol("Z", &end, 36);
    ASSERT_EQ_INT(35, (int)val, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtol_base36_multi(void)
{
    TEST("strtol: base 36 '10'=36");
    char *end;
    long val = strtol("10", &end, 36);
    ASSERT_EQ_INT(36, (int)val, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

/* ========================================================================= */
/*  strtol: base 0 auto-detect                                              */
/* ========================================================================= */

static void test_strtol_base0_decimal(void)
{
    TEST("strtol: base 0 auto-detect decimal");
    char *end;
    long val = strtol("42", &end, 0);
    ASSERT_EQ_INT(42, (int)val, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtol_base0_hex(void)
{
    TEST("strtol: base 0 auto-detect 0x hex");
    char *end;
    long val = strtol("0x1A", &end, 0);
    ASSERT_EQ_INT(26, (int)val, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtol_base0_octal(void)
{
    TEST("strtol: base 0 auto-detect 0 octal");
    char *end;
    long val = strtol("010", &end, 0);
    ASSERT_EQ_INT(8, (int)val, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtol_base0_zero(void)
{
    TEST("strtol: base 0 plain '0'");
    char *end;
    long val = strtol("0", &end, 0);
    ASSERT_EQ_INT(0, (int)val, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

/* ========================================================================= */
/*  strtol: endptr and partial conversion                                    */
/* ========================================================================= */

static void test_strtol_trailing_garbage(void)
{
    TEST("strtol: trailing non-digits");
    char *end;
    long val = strtol("123abc", &end, 10);
    ASSERT_EQ_INT(123, (int)val, "value");
    ASSERT(*end == 'a', "endptr at first invalid");
    PASS();
}

static void test_strtol_no_conversion(void)
{
    TEST("strtol: no valid digits, endptr == nptr");
    const char *str = "abc";
    char *end;
    long val = strtol(str, &end, 10);
    ASSERT_EQ_INT(0, (int)val, "returns 0");
    ASSERT(end == str, "endptr == nptr");
    PASS();
}

static void test_strtol_empty_string(void)
{
    TEST("strtol: empty string, no conversion");
    const char *str = "";
    char *end;
    long val = strtol(str, &end, 10);
    ASSERT_EQ_INT(0, (int)val, "returns 0");
    ASSERT(end == str, "endptr == nptr");
    PASS();
}

static void test_strtol_only_whitespace(void)
{
    TEST("strtol: only whitespace, no conversion");
    const char *str = "   ";
    char *end;
    long val = strtol(str, &end, 10);
    ASSERT_EQ_INT(0, (int)val, "returns 0");
    ASSERT(end == str, "endptr == nptr");
    PASS();
}

static void test_strtol_only_sign(void)
{
    TEST("strtol: only '+', no conversion");
    const char *str = "+";
    char *end;
    long val = strtol(str, &end, 10);
    ASSERT_EQ_INT(0, (int)val, "returns 0");
    ASSERT(end == str, "endptr == nptr");
    PASS();
}

static void test_strtol_only_minus(void)
{
    TEST("strtol: only '-', no conversion");
    const char *str = "-";
    char *end;
    long val = strtol(str, &end, 10);
    ASSERT_EQ_INT(0, (int)val, "returns 0");
    ASSERT(end == str, "endptr == nptr");
    PASS();
}

static void test_strtol_no_conversion_errno_preserved(void)
{
    TEST("strtol: no conversion preserves errno");
    errno = EDOM;
    strtol("abc", NULL, 10);
    ASSERT_EQ_INT(EDOM, errno, "errno preserved on no conversion");
    PASS();
}

/* ========================================================================= */
/*  strtol: prefix edge cases                                                */
/* ========================================================================= */

/*
 * Incomplete "0x" prefix behavior:
 * The C standard says the subject sequence is the longest initial subsequence
 * of the expected form. When "0x" has no valid hex digit following it, some
 * implementations (glibc) parse "0" and set endptr to 'x', while others (UCRT)
 * treat the entire token as a failed prefix and set endptr to the start.
 * These tests accept either behavior.
 */

static void test_strtol_incomplete_0x_base0(void)
{
    TEST("strtol: '0x' with base 0, parses 0");
    const char *s = "0x";
    char *end;
    long val = strtol(s, &end, 0);
    ASSERT_EQ_INT(0, (int)val, "value is 0");
    ASSERT(end == s || *end == 'x', "endptr at start or 'x'");
    PASS();
}

static void test_strtol_incomplete_0x_base16(void)
{
    TEST("strtol: '0x' with base 16, parses 0");
    const char *s = "0x";
    char *end;
    long val = strtol(s, &end, 16);
    ASSERT_EQ_INT(0, (int)val, "value is 0");
    ASSERT(end == s || *end == 'x', "endptr at start or 'x'");
    PASS();
}

static void test_strtol_0xG_base16(void)
{
    TEST("strtol: '0xG' with base 16, parses 0");
    const char *s = "0xG";
    char *end;
    long val = strtol(s, &end, 16);
    ASSERT_EQ_INT(0, (int)val, "value is 0");
    ASSERT(end == s || *end == 'x', "endptr at start or 'x'");
    PASS();
}

static void test_strtol_octal_invalid_digit(void)
{
    TEST("strtol: '08' with base 0, stops at '8'");
    char *end;
    long val = strtol("08", &end, 0);
    ASSERT_EQ_INT(0, (int)val, "value is 0");
    ASSERT(*end == '8', "endptr at '8'");
    PASS();
}

static void test_strtol_binary_invalid_digit(void)
{
    TEST("strtol: '102' with base 2, stops at '2'");
    char *end;
    long val = strtol("102", &end, 2);
    ASSERT_EQ_INT(2, (int)val, "10 in binary = 2");
    ASSERT(*end == '2', "endptr at '2'");
    PASS();
}

static void test_strtol_base35_z_invalid(void)
{
    TEST("strtol: 'z' invalid in base 35");
    const char *str = "z";
    char *end;
    long val = strtol(str, &end, 35);
    ASSERT_EQ_INT(0, (int)val, "no conversion");
    ASSERT(end == str, "endptr == nptr");
    PASS();
}

static void test_strtol_signed_hex(void)
{
    TEST("strtol: '-0x10' with base 0 = -16");
    long val = strtol("-0x10", NULL, 0);
    ASSERT_EQ_INT(-16, (int)val, "value");
    PASS();
}

static void test_strtol_signed_octal(void)
{
    TEST("strtol: '-010' with base 0 = -8");
    long val = strtol("-010", NULL, 0);
    ASSERT_EQ_INT(-8, (int)val, "value");
    PASS();
}

/* ========================================================================= */
/*  strtol: errno and overflow/underflow                                     */
/* ========================================================================= */

static void test_strtol_errno_preserved_on_success(void)
{
    TEST("strtol: errno unchanged on success");
    errno = EDOM;
    strtol("123", NULL, 10);
    ASSERT_EQ_INT(EDOM, errno, "errno preserved");
    PASS();
}

static void test_strtol_overflow(void)
{
    TEST("strtol: overflow returns LONG_MAX, ERANGE");
    errno = 0;
    char *end;
    long val = strtol("99999999999999999999", &end, 10);
    ASSERT(val == LONG_MAX, "returns LONG_MAX");
    ASSERT_EQ_INT(ERANGE, errno, "errno == ERANGE");
    PASS();
}

static void test_strtol_underflow(void)
{
    TEST("strtol: underflow returns LONG_MIN, ERANGE");
    errno = 0;
    char *end;
    long val = strtol("-99999999999999999999", &end, 10);
    ASSERT(val == LONG_MIN, "returns LONG_MIN");
    ASSERT_EQ_INT(ERANGE, errno, "errno == ERANGE");
    PASS();
}

static void test_strtol_long_max_exact(void)
{
    TEST("strtol: LONG_MAX exact value");
    char buf[32];
    sprintf(buf, "%ld", LONG_MAX);
    errno = 0;
    char *end;
    long val = strtol(buf, &end, 10);
    ASSERT(val == LONG_MAX, "exact LONG_MAX");
    ASSERT(*end == '\0', "consumed all");
    ASSERT(errno != ERANGE, "no ERANGE");
    PASS();
}

static void test_strtol_long_min_exact(void)
{
    TEST("strtol: LONG_MIN exact value");
    char buf[32];
    sprintf(buf, "%ld", LONG_MIN);
    errno = 0;
    char *end;
    long val = strtol(buf, &end, 10);
    ASSERT(val == LONG_MIN, "exact LONG_MIN");
    ASSERT(*end == '\0', "consumed all");
    ASSERT(errno != ERANGE, "no ERANGE");
    PASS();
}

static void test_strtol_long_max_plus_one(void)
{
    TEST("strtol: LONG_MAX+1 as string triggers ERANGE");
    errno = 0;
    char *end;
    long val;
    if (sizeof(long) == 4)
        val = strtol("2147483648", &end, 10);
    else
        val = strtol("9223372036854775808", &end, 10);
    ASSERT(val == LONG_MAX, "returns LONG_MAX");
    ASSERT_EQ_INT(ERANGE, errno, "errno == ERANGE");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtol_long_min_minus_one(void)
{
    TEST("strtol: LONG_MIN-1 as string triggers ERANGE");
    errno = 0;
    char *end;
    long val;
    if (sizeof(long) == 4)
        val = strtol("-2147483649", &end, 10);
    else
        val = strtol("-9223372036854775809", &end, 10);
    ASSERT(val == LONG_MIN, "returns LONG_MIN");
    ASSERT_EQ_INT(ERANGE, errno, "errno == ERANGE");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtol_overflow_endptr(void)
{
    TEST("strtol: overflow sets endptr to end of number");
    errno = 0;
    char *end;
    strtol("99999999999999999999xyz", &end, 10);
    ASSERT_EQ_INT(ERANGE, errno, "ERANGE set");
    ASSERT(*end == 'x', "endptr at first non-digit");
    PASS();
}

/* ========================================================================= */
/*  strtoul: basic and edge cases                                            */
/* ========================================================================= */

static void test_strtoul_basic(void)
{
    TEST("strtoul: basic unsigned");
    char *end;
    unsigned long val = strtoul("42", &end, 10);
    ASSERT_EQ_INT(42, (int)val, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtoul_hex(void)
{
    TEST("strtoul: hex with prefix");
    unsigned long val = strtoul("0xDEAD", NULL, 0);
    ASSERT(val == 0xDEAD, "value");
    PASS();
}

static void test_strtoul_negative_one(void)
{
    TEST("strtoul: '-1' wraps to ULONG_MAX");
    errno = 0;
    unsigned long val = strtoul("-1", NULL, 10);
    ASSERT(val == ULONG_MAX, "wraps to ULONG_MAX");
    PASS();
}

static void test_strtoul_negative_two(void)
{
    TEST("strtoul: '-2' wraps to ULONG_MAX - 1");
    unsigned long val = strtoul("-2", NULL, 10);
    ASSERT(val == ULONG_MAX - 1, "wraps to ULONG_MAX - 1");
    PASS();
}

static void test_strtoul_negative_zero(void)
{
    TEST("strtoul: '-0' = 0");
    unsigned long val = strtoul("-0", NULL, 10);
    ASSERT(val == 0, "negative zero is 0");
    PASS();
}

static void test_strtoul_negative_one_endptr(void)
{
    TEST("strtoul: '-1' sets endptr correctly");
    char *end;
    errno = 0;
    unsigned long val = strtoul("-1", &end, 10);
    ASSERT(val == ULONG_MAX, "wraps to ULONG_MAX");
    ASSERT(*end == '\0', "consumed all");
    ASSERT(errno != ERANGE, "no ERANGE for valid negative");
    PASS();
}

static void test_strtoul_errno_preserved(void)
{
    TEST("strtoul: errno unchanged on success");
    errno = EDOM;
    strtoul("42", NULL, 10);
    ASSERT_EQ_INT(EDOM, errno, "errno preserved");
    PASS();
}

static void test_strtoul_overflow(void)
{
    TEST("strtoul: overflow returns ULONG_MAX, ERANGE");
    errno = 0;
    unsigned long val = strtoul("99999999999999999999", NULL, 10);
    ASSERT(val == ULONG_MAX, "returns ULONG_MAX");
    ASSERT_EQ_INT(ERANGE, errno, "errno == ERANGE");
    PASS();
}

static void test_strtoul_ulong_max_exact(void)
{
    TEST("strtoul: ULONG_MAX exact value");
    char buf[32];
    sprintf(buf, "%lu", ULONG_MAX);
    errno = 0;
    char *end;
    unsigned long val = strtoul(buf, &end, 10);
    ASSERT(val == ULONG_MAX, "exact ULONG_MAX");
    ASSERT(*end == '\0', "consumed all");
    ASSERT(errno != ERANGE, "no ERANGE");
    PASS();
}

static void test_strtoul_ulong_max_plus_one(void)
{
    TEST("strtoul: ULONG_MAX+1 as string triggers ERANGE");
    errno = 0;
    char *end;
    unsigned long val;
    if (sizeof(unsigned long) == 4)
        val = strtoul("4294967296", &end, 10);
    else
        val = strtoul("18446744073709551616", &end, 10);
    ASSERT(val == ULONG_MAX, "returns ULONG_MAX");
    ASSERT_EQ_INT(ERANGE, errno, "errno == ERANGE");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtoul_no_conversion(void)
{
    TEST("strtoul: no valid digits, endptr == nptr");
    const char *str = "xyz";
    char *end;
    unsigned long val = strtoul(str, &end, 10);
    ASSERT(val == 0, "returns 0");
    ASSERT(end == str, "endptr == nptr");
    PASS();
}

static void test_strtoul_trailing_garbage(void)
{
    TEST("strtoul: stops at non-digit");
    char *end;
    unsigned long val = strtoul("42abc", &end, 10);
    ASSERT(val == 42, "value");
    ASSERT(*end == 'a', "endptr at 'a'");
    PASS();
}

/* ========================================================================= */
/*  strtoll / strtoull                                                       */
/* ========================================================================= */

static void test_strtoll_basic(void)
{
    TEST("strtoll: basic conversion");
    char *end;
    long long val = strtoll("123456789", &end, 10);
    ASSERT(val == 123456789LL, "value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtoll_negative(void)
{
    TEST("strtoll: negative");
    long long val = strtoll("-987654321", NULL, 10);
    ASSERT(val == -987654321LL, "value");
    PASS();
}

static void test_strtoll_errno_preserved(void)
{
    TEST("strtoll: errno unchanged on success");
    errno = EDOM;
    strtoll("42", NULL, 10);
    ASSERT_EQ_INT(EDOM, errno, "errno preserved");
    PASS();
}

static void test_strtoll_overflow(void)
{
    TEST("strtoll: overflow returns LLONG_MAX, ERANGE");
    errno = 0;
    long long val = strtoll("99999999999999999999999", NULL, 10);
    ASSERT(val == LLONG_MAX, "returns LLONG_MAX");
    ASSERT_EQ_INT(ERANGE, errno, "errno == ERANGE");
    PASS();
}

static void test_strtoll_underflow(void)
{
    TEST("strtoll: underflow returns LLONG_MIN, ERANGE");
    errno = 0;
    long long val = strtoll("-99999999999999999999999", NULL, 10);
    ASSERT(val == LLONG_MIN, "returns LLONG_MIN");
    ASSERT_EQ_INT(ERANGE, errno, "errno == ERANGE");
    PASS();
}

static void test_strtoll_llong_max_exact(void)
{
    TEST("strtoll: LLONG_MAX exact value");
    errno = 0;
    char *end;
    long long val = strtoll("9223372036854775807", &end, 10);
    ASSERT(val == LLONG_MAX, "exact LLONG_MAX");
    ASSERT(*end == '\0', "consumed all");
    ASSERT(errno != ERANGE, "no ERANGE");
    PASS();
}

static void test_strtoll_llong_min_exact(void)
{
    TEST("strtoll: LLONG_MIN exact value");
    errno = 0;
    char *end;
    long long val = strtoll("-9223372036854775808", &end, 10);
    ASSERT(val == LLONG_MIN, "exact LLONG_MIN");
    ASSERT(*end == '\0', "consumed all");
    ASSERT(errno != ERANGE, "no ERANGE");
    PASS();
}

static void test_strtoull_basic(void)
{
    TEST("strtoull: basic conversion");
    char *end;
    unsigned long long val = strtoull("18446744073709551615", &end, 10);
    ASSERT(val == ULLONG_MAX, "exact ULLONG_MAX");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtoull_errno_preserved(void)
{
    TEST("strtoull: errno unchanged on success");
    errno = EDOM;
    strtoull("42", NULL, 10);
    ASSERT_EQ_INT(EDOM, errno, "errno preserved");
    PASS();
}

static void test_strtoull_overflow(void)
{
    TEST("strtoull: overflow returns ULLONG_MAX, ERANGE");
    errno = 0;
    unsigned long long val = strtoull("99999999999999999999999", NULL, 10);
    ASSERT(val == ULLONG_MAX, "returns ULLONG_MAX");
    ASSERT_EQ_INT(ERANGE, errno, "errno == ERANGE");
    PASS();
}

static void test_strtoull_ullong_max_exact(void)
{
    TEST("strtoull: ULLONG_MAX exact value");
    errno = 0;
    char *end;
    long long val = strtoull("18446744073709551615", &end, 10);
    ASSERT(val == ULLONG_MAX, "exact ULLONG_MAX");
    ASSERT(*end == '\0', "consumed all");
    ASSERT(errno != ERANGE, "no ERANGE");
    PASS();
}

static void test_strtoull_ullong_max_plus_one(void)
{
    TEST("strtoull: ULLONG_MAX+1 as string triggers ERANGE");
    errno = 0;
    char *end;
    unsigned long long val = strtoull("18446744073709551616", &end, 10);
    ASSERT(val == ULLONG_MAX, "returns ULLONG_MAX");
    ASSERT_EQ_INT(ERANGE, errno, "errno == ERANGE");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtoull_no_conversion(void)
{
    TEST("strtoull: no valid digits, endptr == nptr");
    const char *str = "xyz";
    char *end;
    unsigned long long val = strtoull(str, &end, 10);
    ASSERT(val == 0, "returns 0");
    ASSERT(end == str, "endptr == nptr");
    PASS();
}

static void test_strtoull_trailing_garbage(void)
{
    TEST("strtoull: stops at non-digit");
    char *end;
    unsigned long long val = strtoull("42abc", &end, 10);
    ASSERT(val == 42, "value");
    ASSERT(*end == 'a', "endptr at 'a'");
    PASS();
}

static void test_strtoull_negative_one(void)
{
    TEST("strtoull: '-1' wraps to ULLONG_MAX");
    unsigned long long val = strtoull("-1", NULL, 10);
    ASSERT(val == ULLONG_MAX, "wraps to ULLONG_MAX");
    PASS();
}

static void test_strtoull_negative_one_endptr(void)
{
    TEST("strtoull: '-1' sets endptr correctly");
    char *end;
    errno = 0;
    unsigned long long val = strtoull("-1", &end, 10);
    ASSERT(val == ULLONG_MAX, "wraps to ULLONG_MAX");
    ASSERT(*end == '\0', "consumed all");
    ASSERT(errno != ERANGE, "no ERANGE for valid negative");
    PASS();
}

#if TEST_ATOI
/* ========================================================================= */
/*  atoi / atol / atoll                                                      */
/* ========================================================================= */

static void test_atoi_basic(void)
{
    TEST("atoi: basic conversion");
    ASSERT_EQ_INT(42, atoi("42"), "value");
    PASS();
}

static void test_atoi_negative(void)
{
    TEST("atoi: negative");
    ASSERT_EQ_INT(-7, atoi("-7"), "value");
    PASS();
}

static void test_atoi_zero(void)
{
    TEST("atoi: zero");
    ASSERT_EQ_INT(0, atoi("0"), "value");
    PASS();
}

static void test_atoi_leading_whitespace(void)
{
    TEST("atoi: leading whitespace");
    ASSERT_EQ_INT(99, atoi("  99"), "value");
    PASS();
}

static void test_atoi_trailing_garbage(void)
{
    TEST("atoi: trailing garbage ignored");
    ASSERT_EQ_INT(123, atoi("123abc"), "value");
    PASS();
}

static void test_atol_basic(void)
{
    TEST("atol: basic conversion");
    ASSERT(atol("100000") == 100000L, "value");
    PASS();
}

static void test_atol_negative(void)
{
    TEST("atol: negative");
    ASSERT(atol("-54321") == -54321L, "value");
    PASS();
}

static void test_atol_leading_whitespace(void)
{
    TEST("atol: leading whitespace");
    ASSERT(atol("  \t42") == 42L, "value");
    PASS();
}

static void test_atol_trailing_garbage(void)
{
    TEST("atol: trailing garbage ignored");
    ASSERT(atol("100abc") == 100L, "value");
    PASS();
}

static void test_atoll_basic(void)
{
    TEST("atoll: basic conversion");
    ASSERT(atoll("9223372036854775807") == LLONG_MAX, "LLONG_MAX");
    PASS();
}

static void test_atoll_negative(void)
{
    TEST("atoll: negative");
    ASSERT(atoll("-9223372036854775807") == -9223372036854775807LL, "value");
    PASS();
}

static void test_atoll_leading_whitespace(void)
{
    TEST("atoll: leading whitespace");
    ASSERT(atoll("  \t99") == 99LL, "value");
    PASS();
}

static void test_atoll_trailing_garbage(void)
{
    TEST("atoll: trailing garbage ignored");
    ASSERT(atoll("500xyz") == 500LL, "value");
    PASS();
}
#endif

/* ========================================================================= */
/*  strtoul: prefix edge cases                                               */
/* ========================================================================= */

static void test_strtoul_overflow_endptr(void)
{
    TEST("strtoul: overflow sets endptr to end of number");
    errno = 0;
    char *end;
    strtoul("99999999999999999999xyz", &end, 10);
    ASSERT_EQ_INT(ERANGE, errno, "ERANGE set");
    ASSERT(*end == 'x', "endptr at first non-digit");
    PASS();
}

static void test_strtoul_no_conversion_errno_preserved(void)
{
    TEST("strtoul: no conversion preserves errno");
    errno = EDOM;
    strtoul("abc", NULL, 10);
    ASSERT_EQ_INT(EDOM, errno, "errno preserved on no conversion");
    PASS();
}

/* ========================================================================= */
/*  strtoll: prefix edge cases and boundaries                                */
/* ========================================================================= */

static void test_strtoll_overflow_endptr(void)
{
    TEST("strtoll: overflow sets endptr to end of number");
    errno = 0;
    char *end;
    strtoll("99999999999999999999999xyz", &end, 10);
    ASSERT_EQ_INT(ERANGE, errno, "ERANGE set");
    ASSERT(*end == 'x', "endptr at first non-digit");
    PASS();
}

static void test_strtoll_no_conversion_errno_preserved(void)
{
    TEST("strtoll: no conversion preserves errno");
    errno = EDOM;
    strtoll("abc", NULL, 10);
    ASSERT_EQ_INT(EDOM, errno, "errno preserved on no conversion");
    PASS();
}

static void test_strtoll_llong_max_plus_one(void)
{
    TEST("strtoll: LLONG_MAX+1 as string triggers ERANGE");
    errno = 0;
    char *end;
    long long val = strtoll("9223372036854775808", &end, 10);
    ASSERT(val == LLONG_MAX, "returns LLONG_MAX");
    ASSERT_EQ_INT(ERANGE, errno, "errno == ERANGE");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtoll_llong_min_minus_one(void)
{
    TEST("strtoll: LLONG_MIN-1 as string triggers ERANGE");
    errno = 0;
    char *end;
    long long val = strtoll("-9223372036854775809", &end, 10);
    ASSERT(val == LLONG_MIN, "returns LLONG_MIN");
    ASSERT_EQ_INT(ERANGE, errno, "errno == ERANGE");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

/* ========================================================================= */
/*  strtoull: prefix edge cases                                              */
/* ========================================================================= */

static void test_strtoull_overflow_endptr(void)
{
    TEST("strtoull: overflow sets endptr to end of number");
    errno = 0;
    char *end;
    strtoull("99999999999999999999999xyz", &end, 10);
    ASSERT_EQ_INT(ERANGE, errno, "ERANGE set");
    ASSERT(*end == 'x', "endptr at first non-digit");
    PASS();
}

static void test_strtoull_no_conversion_errno_preserved(void)
{
    TEST("strtoull: no conversion preserves errno");
    errno = EDOM;
    strtoull("abc", NULL, 10);
    ASSERT_EQ_INT(EDOM, errno, "errno preserved on no conversion");
    PASS();
}

#if TEST_STRTOIMAX
/* ========================================================================= */
/*  strtoimax / strtoumax                                                    */
/* ========================================================================= */

static void test_strtoimax_basic(void)
{
    TEST("strtoimax: basic positive");
    char *end;
    intmax_t val = strtoimax("12345", &end, 10);
    ASSERT(val == 12345, "parsed value");
    ASSERT(*end == '\0', "consumed all");
    PASS();
}

static void test_strtoimax_negative(void)
{
    TEST("strtoimax: negative value");
    intmax_t val = strtoimax("-999", NULL, 10);
    ASSERT(val == -999, "parsed negative");
    PASS();
}

static void test_strtoimax_hex(void)
{
    TEST("strtoimax: hex with base 16");
    intmax_t val = strtoimax("ff", NULL, 16);
    ASSERT(val == 255, "hex ff = 255");
    PASS();
}

static void test_strtoimax_intmax_max(void)
{
    TEST("strtoimax: INTMAX_MAX exact");
    char buf[32];
    sprintf(buf, "%jd", INTMAX_MAX);
    errno = 0;
    intmax_t val = strtoimax(buf, NULL, 10);
    ASSERT(val == INTMAX_MAX, "exact INTMAX_MAX");
    ASSERT(errno == 0, "no overflow");
    PASS();
}

static void test_strtoimax_overflow(void)
{
    TEST("strtoimax: overflow sets ERANGE");
    errno = 0;
    intmax_t val = strtoimax("99999999999999999999999999", NULL, 10);
    ASSERT(errno == ERANGE, "ERANGE set");
    ASSERT(val == INTMAX_MAX, "returns INTMAX_MAX");
    PASS();
}

static void test_strtoimax_underflow(void)
{
    TEST("strtoimax: underflow sets ERANGE");
    errno = 0;
    intmax_t val = strtoimax("-99999999999999999999999999", NULL, 10);
    ASSERT(errno == ERANGE, "ERANGE set");
    ASSERT(val == INTMAX_MIN, "returns INTMAX_MIN");
    PASS();
}

static void test_strtoimax_endptr(void)
{
    TEST("strtoimax: endptr points to first invalid char");
    char *end;
    strtoimax("42abc", &end, 10);
    ASSERT(*end == 'a', "stops at 'a'");
    PASS();
}

static void test_strtoumax_basic(void)
{
    TEST("strtoumax: basic positive");
    uintmax_t val = strtoumax("12345", NULL, 10);
    ASSERT(val == 12345, "parsed value");
    PASS();
}

static void test_strtoumax_uintmax_max(void)
{
    TEST("strtoumax: UINTMAX_MAX exact");
    char buf[32];
    sprintf(buf, "%ju", UINTMAX_MAX);
    errno = 0;
    uintmax_t val = strtoumax(buf, NULL, 10);
    ASSERT(val == UINTMAX_MAX, "exact UINTMAX_MAX");
    ASSERT(errno == 0, "no overflow");
    PASS();
}

static void test_strtoumax_overflow(void)
{
    TEST("strtoumax: overflow sets ERANGE");
    errno = 0;
    uintmax_t val = strtoumax("99999999999999999999999999", NULL, 10);
    ASSERT(errno == ERANGE, "ERANGE set");
    ASSERT(val == UINTMAX_MAX, "returns UINTMAX_MAX");
    PASS();
}
#endif

/* ========================================================================= */
/*  run_strtol_tests                                                         */
/* ========================================================================= */

TEST_SUITE(strtol, "strtol-family tests")
{
    printf(COLOR_YELLOW "[strtol: basic]" COLOR_RESET "\n");
    test_strtol_positive();
    test_strtol_negative();
    test_strtol_zero();
    test_strtol_explicit_plus();
    test_strtol_null_endptr();

    printf(COLOR_YELLOW "[strtol: whitespace]" COLOR_RESET "\n");
    test_strtol_leading_spaces();
    test_strtol_leading_whitespace_mix();

    printf(COLOR_YELLOW "[strtol: bases]" COLOR_RESET "\n");
    test_strtol_base16();
    test_strtol_base16_prefix();
    test_strtol_base16_uppercase();
    test_strtol_base8();
    test_strtol_base8_prefix();
    test_strtol_base2();
    test_strtol_base36();
    test_strtol_base36_uppercase();
    test_strtol_base36_multi();

    printf(COLOR_YELLOW "[strtol: base 0 auto-detect]" COLOR_RESET "\n");
    test_strtol_base0_decimal();
    test_strtol_base0_hex();
    test_strtol_base0_octal();
    test_strtol_base0_zero();

    printf(COLOR_YELLOW "[strtol: endptr / partial conversion]" COLOR_RESET "\n");
    test_strtol_trailing_garbage();
    test_strtol_no_conversion();
    test_strtol_empty_string();
    test_strtol_only_whitespace();
    test_strtol_only_sign();
    test_strtol_only_minus();
    test_strtol_no_conversion_errno_preserved();

    printf(COLOR_YELLOW "[strtol: prefix edge cases]" COLOR_RESET "\n");
    test_strtol_incomplete_0x_base0();
    test_strtol_incomplete_0x_base16();
    test_strtol_0xG_base16();
    test_strtol_octal_invalid_digit();
    test_strtol_binary_invalid_digit();
    test_strtol_base35_z_invalid();
    test_strtol_signed_hex();
    test_strtol_signed_octal();

    printf(COLOR_YELLOW "[strtol: errno / overflow]" COLOR_RESET "\n");
    test_strtol_errno_preserved_on_success();
    test_strtol_overflow();
    test_strtol_underflow();
    test_strtol_long_max_exact();
    test_strtol_long_min_exact();
    test_strtol_long_max_plus_one();
    test_strtol_long_min_minus_one();
    test_strtol_overflow_endptr();

    printf(COLOR_YELLOW "[strtoul]" COLOR_RESET "\n");
    test_strtoul_basic();
    test_strtoul_hex();
    test_strtoul_negative_one();
    test_strtoul_negative_two();
    test_strtoul_negative_zero();
    test_strtoul_negative_one_endptr();
    test_strtoul_errno_preserved();
    test_strtoul_overflow();
    test_strtoul_ulong_max_exact();
    test_strtoul_ulong_max_plus_one();
    test_strtoul_no_conversion();
    test_strtoul_trailing_garbage();

    printf(COLOR_YELLOW "[strtoll / strtoull]" COLOR_RESET "\n");
    test_strtoll_basic();
    test_strtoll_negative();
    test_strtoll_errno_preserved();
    test_strtoll_overflow();
    test_strtoll_underflow();
    test_strtoll_llong_max_exact();
    test_strtoll_llong_min_exact();
    test_strtoull_basic();
    test_strtoull_errno_preserved();
    test_strtoull_overflow();
    test_strtoull_ullong_max_exact();
    test_strtoull_ullong_max_plus_one();
    test_strtoull_no_conversion();
    test_strtoull_trailing_garbage();
    test_strtoull_negative_one();
    test_strtoull_negative_one_endptr();

    printf(COLOR_YELLOW "[strtoul: prefix edge cases]" COLOR_RESET "\n");
    test_strtoul_overflow_endptr();
    test_strtoul_no_conversion_errno_preserved();

    printf(COLOR_YELLOW "[strtoll: prefix edge cases / boundaries]" COLOR_RESET "\n");
    test_strtoll_overflow_endptr();
    test_strtoll_no_conversion_errno_preserved();
    test_strtoll_llong_max_plus_one();
    test_strtoll_llong_min_minus_one();

    printf(COLOR_YELLOW "[strtoull: prefix edge cases]" COLOR_RESET "\n");
    test_strtoull_overflow_endptr();
    test_strtoull_no_conversion_errno_preserved();

#if TEST_ATOI
    printf(COLOR_YELLOW "[atoi / atol / atoll]" COLOR_RESET "\n");
    test_atoi_basic();
    test_atoi_negative();
    test_atoi_zero();
    test_atoi_leading_whitespace();
    test_atoi_trailing_garbage();
    test_atol_basic();
    test_atol_negative();
    test_atol_leading_whitespace();
    test_atol_trailing_garbage();
    test_atoll_basic();
    test_atoll_negative();
    test_atoll_leading_whitespace();
    test_atoll_trailing_garbage();
#endif

#if TEST_STRTOIMAX
    printf(COLOR_YELLOW "[strtoimax / strtoumax]" COLOR_RESET "\n");
    test_strtoimax_basic();
    test_strtoimax_negative();
    test_strtoimax_hex();
    test_strtoimax_intmax_max();
    test_strtoimax_overflow();
    test_strtoimax_underflow();
    test_strtoimax_endptr();
    test_strtoumax_basic();
    test_strtoumax_uintmax_max();
    test_strtoumax_overflow();
#endif
}
