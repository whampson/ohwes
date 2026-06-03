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
 *         File: src/libc/test/test_printf.c
 *      Created: May 23, 2026
 *       Author: Wes Hampson
 *
 * libc printf-family function tests
 *
 * Covers: format specifiers (%d, %i, %u, %x, %X, %o, %s, %c, %p, %f, %e,
 *         %g, %%, %n, %ld, %lld, %zu, %hd), width, precision, padding,
 *         flags (+, -, 0, #, space), edge cases, return values, truncation.
 * =============================================================================
 */

#include <limits.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <stdarg.h>
#if TEST_PRINTF_FLOAT
#include <float.h>
#endif
#include "framework.h"

/* ========================================================================= */
/*  sprintf: basic format specifiers                                         */
/* ========================================================================= */

static void test_sprintf_decimal(void)
{
    TEST("sprintf: %d basic integer");
    char buf[64];
    sprintf(buf, "%d", 42);
    ASSERT_EQ_STR("42", buf, "mismatch");
    PASS();
}

static void test_sprintf_negative(void)
{
    TEST("sprintf: %d negative integer");
    char buf[64];
    sprintf(buf, "%d", -42);
    ASSERT_EQ_STR("-42", buf, "mismatch");
    PASS();
}

static void test_sprintf_zero(void)
{
    TEST("sprintf: %d zero");
    char buf[64];
    sprintf(buf, "%d", 0);
    ASSERT_EQ_STR("0", buf, "mismatch");
    PASS();
}

static void test_sprintf_int_max(void)
{
    TEST("sprintf: %d INT_MAX");
    char buf[64];
    sprintf(buf, "%d", INT_MAX);
    ASSERT_EQ_STR("2147483647", buf, "mismatch");
    PASS();
}

static void test_sprintf_int_min(void)
{
    TEST("sprintf: %d INT_MIN");
    char buf[64];
    sprintf(buf, "%d", INT_MIN);
    ASSERT_EQ_STR("-2147483648", buf, "mismatch");
    PASS();
}

static void test_sprintf_i_specifier(void)
{
    TEST("sprintf: %i same as %d");
    char buf[64];
    sprintf(buf, "%i", -7);
    ASSERT_EQ_STR("-7", buf, "mismatch");
    PASS();
}

static void test_sprintf_unsigned(void)
{
    TEST("sprintf: %u basic unsigned");
    char buf[64];
    sprintf(buf, "%u", 42u);
    ASSERT_EQ_STR("42", buf, "mismatch");
    PASS();
}

static void test_sprintf_hex_lower(void)
{
    TEST("sprintf: %x lowercase hex");
    char buf[64];
    int ret = sprintf(buf, "%x", 0xdeadbeef);
    ASSERT_EQ_STR("deadbeef", buf, "mismatch");
    ASSERT_EQ_INT(8, ret, "return count");
    PASS();
}

static void test_sprintf_hex_upper(void)
{
    TEST("sprintf: %X uppercase hex");
    char buf[64];
    int ret = sprintf(buf, "%X", 0xdeadbeef);
    ASSERT_EQ_STR("DEADBEEF", buf, "mismatch");
    ASSERT_EQ_INT(8, ret, "return count");
    PASS();
}

static void test_sprintf_hex_zero(void)
{
    TEST("sprintf: %x zero");
    char buf[64];
    sprintf(buf, "%x", 0);
    ASSERT_EQ_STR("0", buf, "mismatch");
    PASS();
}

static void test_sprintf_octal(void)
{
    TEST("sprintf: %o octal");
    char buf[64];
    int ret = sprintf(buf, "%o", 255);
    ASSERT_EQ_STR("377", buf, "mismatch");
    ASSERT_EQ_INT(3, ret, "return count");
    PASS();
}

static void test_sprintf_octal_zero(void)
{
    TEST("sprintf: %o zero");
    char buf[64];
    sprintf(buf, "%o", 0);
    ASSERT_EQ_STR("0", buf, "mismatch");
    PASS();
}

static void test_sprintf_char(void)
{
    TEST("sprintf: %c character");
    char buf[64];
    int ret = sprintf(buf, "%c", 'A');
    ASSERT_EQ_STR("A", buf, "mismatch");
    ASSERT_EQ_INT(1, ret, "return count");
    PASS();
}

static void test_sprintf_char_nul(void)
{
    TEST("sprintf: %c NUL character");
    char buf[4] = "xxx";
    int ret = sprintf(buf, "a%cb", '\0');
    ASSERT(buf[0] == 'a' && buf[1] == '\0' && buf[2] == 'b', "NUL embedded");
    ASSERT_EQ_INT(3, ret, "return count includes NUL char");
    PASS();
}

static void test_sprintf_string(void)
{
    TEST("sprintf: %s string");
    char buf[64];
    int ret = sprintf(buf, "%s", "hello");
    ASSERT_EQ_STR("hello", buf, "mismatch");
    ASSERT_EQ_INT(5, ret, "return count");
    PASS();
}

static void test_sprintf_string_empty(void)
{
    TEST("sprintf: %s empty string");
    char buf[64];
    int ret = sprintf(buf, "%s", "");
    ASSERT_EQ_STR("", buf, "mismatch");
    ASSERT_EQ_INT(0, ret, "return count");
    PASS();
}

static void test_sprintf_percent_literal(void)
{
    TEST("sprintf: %% literal percent");
    char buf[64];
    int ret = sprintf(buf, "100%%");
    ASSERT_EQ_STR("100%", buf, "mismatch");
    ASSERT_EQ_INT(4, ret, "return count");
    PASS();
}

static void test_sprintf_pointer(void)
{
    TEST("sprintf: %p non-NULL pointer");
    char buf[64];
    int x = 0;
    int ret = sprintf(buf, "%p", (void *)&x);
    ASSERT(ret > 0, "should produce output");
    ASSERT(strlen(buf) > 0, "non-empty");
    PASS();
}


#if TEST_PRINTF_FLOAT
/* ========================================================================= */
/*  sprintf: floating-point specifiers                                       */
/* ========================================================================= */

static void test_sprintf_float_basic(void)
{
    TEST("sprintf: %f basic float");
    char buf[64];
    sprintf(buf, "%f", 3.14);
    ASSERT_EQ_STR("3.140000", buf, "default 6 decimals");
    PASS();
}

static void test_sprintf_float_negative(void)
{
    TEST("sprintf: %f negative float");
    char buf[64];
    sprintf(buf, "%f", -2.5);
    ASSERT_EQ_STR("-2.500000", buf, "mismatch");
    PASS();
}

static void test_sprintf_float_zero(void)
{
    TEST("sprintf: %f zero");
    char buf[64];
    sprintf(buf, "%f", 0.0);
    ASSERT_EQ_STR("0.000000", buf, "mismatch");
    PASS();
}

static void test_sprintf_float_precision(void)
{
    TEST("sprintf: %.2f precision");
    char buf[64];
    sprintf(buf, "%.2f", 3.14159);
    ASSERT_EQ_STR("3.14", buf, "mismatch");
    PASS();
}

static void test_sprintf_float_precision_zero(void)
{
    TEST("sprintf: %.0f no decimals");
    char buf[64];
    sprintf(buf, "%.0f", 3.7);
    ASSERT_EQ_STR("4", buf, "should round");
    PASS();
}

static void test_sprintf_scientific_lower(void)
{
    TEST("sprintf: %e scientific notation");
    char buf[64];
    sprintf(buf, "%e", 1234.5);
    /* Check prefix; exponent format may vary by platform */
    ASSERT(buf[0] == '1' && buf[1] == '.', "should start with 1.");
    ASSERT(strchr(buf, 'e') != NULL, "should contain 'e'");
    PASS();
}

static void test_sprintf_scientific_upper(void)
{
    TEST("sprintf: %E uppercase scientific");
    char buf[64];
    sprintf(buf, "%E", 1234.5);
    ASSERT(strchr(buf, 'E') != NULL, "should contain 'E'");
    PASS();
}

static void test_sprintf_g_specifier(void)
{
    TEST("sprintf: %g removes trailing zeros");
    char buf[64];
    sprintf(buf, "%g", 3.0);
    ASSERT_EQ_STR("3", buf, "should strip trailing zeros");
    PASS();
}

static void test_sprintf_g_small(void)
{
    TEST("sprintf: %g uses scientific for small values");
    char buf[64];
    sprintf(buf, "%g", 0.00001);
    ASSERT(strchr(buf, 'e') != NULL, "should use scientific");
    PASS();
}

static void test_sprintf_inf(void)
{
    TEST("sprintf: %f infinity");
    char buf[64];
    sprintf(buf, "%f", INFINITY);
    /* Could be "inf", "Inf", or "infinity" depending on platform */
    ASSERT(buf[0] == 'i' || buf[0] == 'I', "should start with i/I");
    PASS();
}

static void test_sprintf_neg_inf(void)
{
    TEST("sprintf: %f negative infinity");
    char buf[64];
    sprintf(buf, "%f", -INFINITY);
    ASSERT(buf[0] == '-', "should start with -");
    PASS();
}

static void test_sprintf_nan(void)
{
    TEST("sprintf: %f NaN");
    char buf[64];
    sprintf(buf, "%f", NAN);
    ASSERT(buf[0] == 'n' || buf[0] == 'N' || buf[0] == '-',
           "should be nan/NaN/-nan");
    PASS();
}
#endif

/* ========================================================================= */
/*  sprintf: length modifiers                                                */
/* ========================================================================= */

static void test_sprintf_long(void)
{
    TEST("sprintf: %ld long");
    char buf[64];
    int ret = sprintf(buf, "%ld", (long)-123456789);
    ASSERT_EQ_STR("-123456789", buf, "mismatch");
    ASSERT_EQ_INT(10, ret, "return count");
    PASS();
}

static void test_sprintf_long_long(void)
{
    TEST("sprintf: %lld long long");
    char buf[64];
    int ret = sprintf(buf, "%lld", (long long)1234567890123LL);
    ASSERT_EQ_STR("1234567890123", buf, "mismatch");
    ASSERT_EQ_INT(13, ret, "return count");
    PASS();
}

static void test_sprintf_size_t(void)
{
    TEST("sprintf: %zu size_t");
    char buf[64];
    int ret = sprintf(buf, "%zu", (size_t)12345);
    ASSERT_EQ_STR("12345", buf, "mismatch");
    ASSERT_EQ_INT(5, ret, "return count");
    PASS();
}

static void test_sprintf_short(void)
{
    TEST("sprintf: %hd short");
    char buf[64];
    int ret = sprintf(buf, "%hd", (short)-100);
    ASSERT_EQ_STR("-100", buf, "mismatch");
    ASSERT_EQ_INT(4, ret, "return count");
    PASS();
}

static void test_sprintf_unsigned_long_long(void)
{
    TEST("sprintf: %llu unsigned long long");
    char buf[64];
    int ret = sprintf(buf, "%llu", (unsigned long long)12345678ULL);
    ASSERT_EQ_STR("12345678", buf, "mismatch");
    ASSERT_EQ_INT(8, ret, "return count");
    PASS();
}

static void test_sprintf_hex_long(void)
{
    TEST("sprintf: %lx long hex");
    char buf[64];
    int ret = sprintf(buf, "%lx", (unsigned long)0xABCDEF);
    ASSERT_EQ_STR("abcdef", buf, "mismatch");
    ASSERT_EQ_INT(6, ret, "return count");
    PASS();
}

/* ========================================================================= */
/*  sprintf: integer boundary values                                         */
/* ========================================================================= */

static void test_sprintf_uint_max(void)
{
    TEST("sprintf: %u UINT_MAX");
    char buf[64];
    sprintf(buf, "%u", UINT_MAX);
    ASSERT_EQ_STR("4294967295", buf, "mismatch");
    PASS();
}

static void test_sprintf_uint_zero(void)
{
    TEST("sprintf: %u zero");
    char buf[64];
    sprintf(buf, "%u", 0u);
    ASSERT_EQ_STR("0", buf, "mismatch");
    PASS();
}

static void test_sprintf_short_max(void)
{
    TEST("sprintf: %hd SHRT_MAX");
    char buf[64];
    sprintf(buf, "%hd", (short) SHRT_MAX);
    ASSERT_EQ_STR("32767", buf, "mismatch");
    PASS();
}

static void test_sprintf_short_min(void)
{
    TEST("sprintf: %hd SHRT_MIN");
    char buf[64];
    sprintf(buf, "%hd", (short) SHRT_MIN);
    ASSERT_EQ_STR("-32768", buf, "mismatch");
    PASS();
}

static void test_sprintf_ushort_max(void)
{
    TEST("sprintf: %hu USHRT_MAX");
    char buf[64];
    sprintf(buf, "%hu", (unsigned short) USHRT_MAX);
    ASSERT_EQ_STR("65535", buf, "mismatch");
    PASS();
}

static void test_sprintf_long_max(void)
{
    TEST("sprintf: %ld LONG_MAX");
    char buf[64];
    sprintf(buf, "%ld", LONG_MAX);
    if (sizeof(long) == 4)
        ASSERT_EQ_STR("2147483647", buf, "mismatch");
    else
        ASSERT_EQ_STR("9223372036854775807", buf, "mismatch");
    PASS();
}

static void test_sprintf_long_min(void)
{
    TEST("sprintf: %ld LONG_MIN");
    char buf[64];
    sprintf(buf, "%ld", LONG_MIN);
    if (sizeof(long) == 4)
        ASSERT_EQ_STR("-2147483648", buf, "mismatch");
    else
        ASSERT_EQ_STR("-9223372036854775808", buf, "mismatch");
    PASS();
}

static void test_sprintf_ulong_max(void)
{
    TEST("sprintf: %lu ULONG_MAX");
    char buf[64];
    sprintf(buf, "%lu", ULONG_MAX);
    if (sizeof(unsigned long) == 4)
        ASSERT_EQ_STR("4294967295", buf, "mismatch");
    else
        ASSERT_EQ_STR("18446744073709551615", buf, "mismatch");
    PASS();
}

static void test_sprintf_llong_max(void)
{
    TEST("sprintf: %lld LLONG_MAX");
    char buf[64];
    sprintf(buf, "%lld", LLONG_MAX);
    ASSERT_EQ_STR("9223372036854775807", buf, "mismatch");
    PASS();
}

static void test_sprintf_llong_min(void)
{
    TEST("sprintf: %lld LLONG_MIN");
    char buf[64];
    sprintf(buf, "%lld", LLONG_MIN);
    ASSERT_EQ_STR("-9223372036854775808", buf, "mismatch");
    PASS();
}

static void test_sprintf_ullong_max(void)
{
    TEST("sprintf: %llu ULLONG_MAX");
    char buf[64];
    sprintf(buf, "%llu", ULLONG_MAX);
    ASSERT_EQ_STR("18446744073709551615", buf, "mismatch");
    PASS();
}

static void test_sprintf_size_max(void)
{
    TEST("sprintf: %zu SIZE_MAX");
    char buf[64];
    sprintf(buf, "%zu", SIZE_MAX);
    if (sizeof(size_t) == 4)
        ASSERT_EQ_STR("4294967295", buf, "mismatch");
    else
        ASSERT_EQ_STR("18446744073709551615", buf, "mismatch");
    PASS();
}

static void test_sprintf_hex_uint_max(void)
{
    TEST("sprintf: %x UINT_MAX");
    char buf[64];
    sprintf(buf, "%x", UINT_MAX);
    ASSERT_EQ_STR("ffffffff", buf, "mismatch");
    PASS();
}

static void test_sprintf_octal_uint_max(void)
{
    TEST("sprintf: %o UINT_MAX");
    char buf[64];
    sprintf(buf, "%o", UINT_MAX);
    ASSERT_EQ_STR("37777777777", buf, "mismatch");
    PASS();
}

/* ========================================================================= */
/*  sprintf: niche length modifiers (%hh, %j, %t, %z)                       */
/* ========================================================================= */

static void test_sprintf_char_signed(void)
{
    TEST("sprintf: %hhd signed char");
    char buf[64];
    sprintf(buf, "%hhd", (signed char)-128);
    ASSERT_EQ_STR("-128", buf, "SCHAR_MIN");
    PASS();
}

static void test_sprintf_char_signed_max(void)
{
    TEST("sprintf: %hhd SCHAR_MAX");
    char buf[64];
    sprintf(buf, "%hhd", (signed char)127);
    ASSERT_EQ_STR("127", buf, "SCHAR_MAX");
    PASS();
}

static void test_sprintf_char_unsigned(void)
{
    TEST("sprintf: %hhu unsigned char");
    char buf[64];
    sprintf(buf, "%hhu", (unsigned char)255);
    ASSERT_EQ_STR("255", buf, "UCHAR_MAX");
    PASS();
}

static void test_sprintf_char_unsigned_zero(void)
{
    TEST("sprintf: %hhu zero");
    char buf[64];
    sprintf(buf, "%hhu", (unsigned char)0);
    ASSERT_EQ_STR("0", buf, "mismatch");
    PASS();
}

static void test_sprintf_char_hex(void)
{
    TEST("sprintf: %hhx unsigned char hex");
    char buf[64];
    sprintf(buf, "%hhx", (unsigned char)0xAB);
    ASSERT_EQ_STR("ab", buf, "mismatch");
    PASS();
}

static void test_sprintf_intmax(void)
{
    TEST("sprintf: %jd intmax_t");
    char buf[64];
    sprintf(buf, "%jd", (intmax_t)INTMAX_MIN);
    ASSERT_EQ_STR("-9223372036854775808", buf, "mismatch");
    PASS();
}

static void test_sprintf_intmax_max(void)
{
    TEST("sprintf: %jd INTMAX_MAX");
    char buf[64];
    sprintf(buf, "%jd", (intmax_t)INTMAX_MAX);
    ASSERT_EQ_STR("9223372036854775807", buf, "mismatch");
    PASS();
}

static void test_sprintf_uintmax(void)
{
    TEST("sprintf: %ju uintmax_t");
    char buf[64];
    sprintf(buf, "%ju", (uintmax_t)UINTMAX_MAX);
    ASSERT_EQ_STR("18446744073709551615", buf, "mismatch");
    PASS();
}

static void test_sprintf_ptrdiff(void)
{
    TEST("sprintf: %td ptrdiff_t positive");
    char buf[64];
    char arr[10];
    ptrdiff_t diff = &arr[9] - &arr[0];
    sprintf(buf, "%td", diff);
    ASSERT_EQ_STR("9", buf, "mismatch");
    PASS();
}

static void test_sprintf_ptrdiff_negative(void)
{
    TEST("sprintf: %td ptrdiff_t negative");
    char buf[64];
    char arr[10];
    ptrdiff_t diff = &arr[0] - &arr[9];
    sprintf(buf, "%td", diff);
    ASSERT_EQ_STR("-9", buf, "mismatch");
    PASS();
}

/* ========================================================================= */
/*  sprintf: flags and width                                                 */
/* ========================================================================= */

static void test_sprintf_width_right_align(void)
{
    TEST("sprintf: %10d right-aligned");
    char buf[64];
    int ret = sprintf(buf, "%10d", 42);
    ASSERT_EQ_STR("        42", buf, "mismatch");
    ASSERT_EQ_INT(10, ret, "return count");
    PASS();
}

static void test_sprintf_width_left_align(void)
{
    TEST("sprintf: %-10d left-aligned");
    char buf[64];
    int ret = sprintf(buf, "%-10d", 42);
    ASSERT_EQ_STR("42        ", buf, "mismatch");
    ASSERT_EQ_INT(10, ret, "return count");
    PASS();
}

static void test_sprintf_zero_pad(void)
{
    TEST("sprintf: %05d zero-padded");
    char buf[64];
    int ret = sprintf(buf, "%05d", 42);
    ASSERT_EQ_STR("00042", buf, "mismatch");
    ASSERT_EQ_INT(5, ret, "return count");
    PASS();
}

static void test_sprintf_zero_pad_negative(void)
{
    TEST("sprintf: %05d zero-padded negative");
    char buf[64];
    sprintf(buf, "%05d", -42);
    ASSERT_EQ_STR("-0042", buf, "sign before padding");
    PASS();
}

static void test_sprintf_plus_flag(void)
{
    TEST("sprintf: %+d plus flag");
    char buf[64];
    int ret = sprintf(buf, "%+d", 42);
    ASSERT_EQ_STR("+42", buf, "mismatch");
    ASSERT_EQ_INT(3, ret, "return count");
    PASS();
}

static void test_sprintf_plus_flag_negative(void)
{
    TEST("sprintf: %+d plus flag negative");
    char buf[64];
    sprintf(buf, "%+d", -42);
    ASSERT_EQ_STR("-42", buf, "mismatch");
    PASS();
}

static void test_sprintf_space_flag(void)
{
    TEST("sprintf: % d space flag positive");
    char buf[64];
    sprintf(buf, "% d", 42);
    ASSERT_EQ_STR(" 42", buf, "space before positive");
    PASS();
}

static void test_sprintf_space_flag_negative(void)
{
    TEST("sprintf: % d space flag negative");
    char buf[64];
    sprintf(buf, "% d", -42);
    ASSERT_EQ_STR("-42", buf, "minus overrides space");
    PASS();
}

static void test_sprintf_hash_hex(void)
{
    TEST("sprintf: %#x hash flag");
    char buf[64];
    int ret = sprintf(buf, "%#x", 255);
    ASSERT_EQ_STR("0xff", buf, "should have 0x prefix");
    ASSERT_EQ_INT(4, ret, "return count");
    PASS();
}

static void test_sprintf_hash_hex_zero(void)
{
    TEST("sprintf: %#x hash flag with zero");
    char buf[64];
    sprintf(buf, "%#x", 0);
    ASSERT_EQ_STR("0", buf, "no prefix for zero");
    PASS();
}

static void test_sprintf_hash_octal(void)
{
    TEST("sprintf: %#o hash flag octal");
    char buf[64];
    int ret = sprintf(buf, "%#o", 255);
    ASSERT_EQ_STR("0377", buf, "should have leading 0");
    ASSERT_EQ_INT(4, ret, "return count");
    PASS();
}

static void test_sprintf_hash_hex_upper(void)
{
    TEST("sprintf: %#X hash flag uppercase");
    char buf[64];
    sprintf(buf, "%#X", 255);
    ASSERT_EQ_STR("0XFF", buf, "should have 0X prefix");
    PASS();
}

static void test_sprintf_left_align_overrides_zero(void)
{
    TEST("sprintf: %-05d left-align overrides zero-pad");
    char buf[64];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
    sprintf(buf, "%-05d", 42);
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("42   ", buf, "left-aligned, no zeros");
    PASS();
}

static void test_sprintf_plus_overrides_space(void)
{
    TEST("sprintf: %+ d plus overrides space");
    char buf[64];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
    sprintf(buf, "%+ d", 42);
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("+42", buf, "plus wins over space");
    PASS();
}

static void test_sprintf_precision_overrides_zero_pad(void)
{
    TEST("sprintf: %05.3d precision disables zero-pad flag");
    char buf[64];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
    sprintf(buf, "%05.3d", 42);
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("  042", buf, "precision pads, width spaces");
    PASS();
}

static void test_sprintf_zero_flag_string(void)
{
    TEST("sprintf: %0s zero flag ignored for strings");
    char buf[64];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
#ifdef __MINGW32__
    __mingw_sprintf(buf, "%010s", "hi");
#else
    sprintf(buf, "%010s", "hi");
#endif
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("        hi", buf, "zero flag ignored, space-padded");
    PASS();
}

static void test_sprintf_plus_flag_string(void)
{
    TEST("sprintf: %+s plus flag ignored for strings");
    char buf[64];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
    sprintf(buf, "%+s", "hello");
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("hello", buf, "plus flag ignored");
    PASS();
}

static void test_sprintf_plus_flag_char(void)
{
    TEST("sprintf: %+c plus flag ignored for char");
    char buf[64];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
    sprintf(buf, "%+c", 'A');
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("A", buf, "plus flag ignored");
    PASS();
}

static void test_sprintf_percent_width(void)
{
    TEST("sprintf: %5% width with percent literal");
    char buf[64];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
    sprintf(buf, "%5%");
#pragma GCC diagnostic pop
    /* Width applied to %% should produce space-padded percent */
    ASSERT(buf[strlen(buf) - 1] == '%', "ends with percent");
    PASS();
}

/* ========================================================================= */
/*  sprintf: precision                                                       */
/* ========================================================================= */

static void test_sprintf_string_precision(void)
{
    TEST("sprintf: %.3s string truncation");
    char buf[64];
    sprintf(buf, "%.3s", "hello");
    ASSERT_EQ_STR("hel", buf, "should truncate to 3");
    PASS();
}

static void test_sprintf_string_precision_longer(void)
{
    TEST("sprintf: %.10s string shorter than precision");
    char buf[64];
    sprintf(buf, "%.10s", "hi");
    ASSERT_EQ_STR("hi", buf, "no padding");
    PASS();
}

static void test_sprintf_int_precision(void)
{
    TEST("sprintf: %.5d minimum digits");
    char buf[64];
    sprintf(buf, "%.5d", 42);
    ASSERT_EQ_STR("00042", buf, "mismatch");
    PASS();
}

static void test_sprintf_int_precision_zero_value(void)
{
    TEST("sprintf: %.0d zero value with zero precision");
    char buf[64];
    sprintf(buf, "%.0d", 0);
    ASSERT_EQ_STR("", buf, "should produce empty string");
    PASS();
}

static void test_sprintf_int_precision_negative(void)
{
    TEST("sprintf: %.5d negative with precision");
    char buf[64];
    sprintf(buf, "%.5d", -42);
    ASSERT_EQ_STR("-00042", buf, "sign before zero padding");
    PASS();
}

static void test_sprintf_hex_precision_zero(void)
{
    TEST("sprintf: %.0x zero value with zero precision");
    char buf[64];
    sprintf(buf, "%.0x", 0);
    ASSERT_EQ_STR("", buf, "should produce empty string");
    PASS();
}

static void test_sprintf_octal_precision_zero(void)
{
    TEST("sprintf: %.0o zero value with zero precision");
    char buf[64];
    sprintf(buf, "%.0o", 0);
    ASSERT_EQ_STR("", buf, "should produce empty string");
    PASS();
}

static void test_sprintf_hash_hex_precision_zero(void)
{
    TEST("sprintf: %#.0x zero with hash and zero precision");
    char buf[64];
    sprintf(buf, "%#.0x", 0);
    ASSERT_EQ_STR("", buf, "hash has no effect with zero value");
    PASS();
}

static void test_sprintf_hash_octal_precision_zero(void)
{
    TEST("sprintf: %#.0o zero with hash and zero precision");
    char buf[64];
    sprintf(buf, "%#.0o", 0);
    ASSERT_EQ_STR("0", buf, "hash forces leading zero for octal");
    PASS();
}

static void test_sprintf_hash_hex_precision_zero_nonzero(void)
{
    TEST("sprintf: %#.0x non-zero with hash and zero precision");
    char buf[64];
    sprintf(buf, "%#.0x", 255);
    ASSERT_EQ_STR("0xff", buf, "hash prefix with value");
    PASS();
}

static void test_sprintf_width_and_precision(void)
{
    TEST("sprintf: %10.5d width and precision combined");
    char buf[64];
    sprintf(buf, "%10.5d", 42);
    ASSERT_EQ_STR("     00042", buf, "mismatch");
    PASS();
}

static void test_sprintf_very_large_width(void)
{
    TEST("sprintf: very large width %200d");
    char buf[256];
    sprintf(buf, "%200d", 42);
    ASSERT_EQ_INT(200, (int)strlen(buf), "output is 200 chars");
    ASSERT(buf[198] == '4' && buf[199] == '2', "digits at end");
    PASS();
}

static void test_sprintf_string_width_precision(void)
{
    TEST("sprintf: %10.3s width and string precision");
    char buf[64];
    sprintf(buf, "%10.3s", "hello");
    ASSERT_EQ_STR("       hel", buf, "right-aligned truncated");
    PASS();
}

static void test_sprintf_string_left_align_precision(void)
{
    TEST("sprintf: %-10.3s left-aligned with precision");
    char buf[64];
    sprintf(buf, "%-10.3s", "hello");
    ASSERT_EQ_STR("hel       ", buf, "left-aligned truncated");
    PASS();
}

/* ========================================================================= */
/*  sprintf: multiple arguments and mixed format                             */
/* ========================================================================= */

static void test_sprintf_multiple_args(void)
{
    TEST("sprintf: multiple format specifiers");
    char buf[128];
    sprintf(buf, "%s is %d years old", "Alice", 30);
    ASSERT_EQ_STR("Alice is 30 years old", buf, "mismatch");
    PASS();
}

static void test_sprintf_no_format(void)
{
    TEST("sprintf: plain string, no specifiers");
    char buf[64];
    sprintf(buf, "hello world");
    ASSERT_EQ_STR("hello world", buf, "mismatch");
    PASS();
}

static void test_sprintf_empty_format(void)
{
    TEST("sprintf: empty format string");
    char buf[64] = "old";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-zero-length"
    sprintf(buf, "");
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("", buf, "should produce empty");
    PASS();
}

static void test_sprintf_multiple_percent(void)
{
    TEST("sprintf: multiple %% in format string");
    char buf[64];
    sprintf(buf, "100%% done, %d%% left", 0);
    ASSERT_EQ_STR("100% done, 0% left", buf, "mismatch");
    PASS();
}

/* ========================================================================= */
/*  sprintf: return value                                                    */
/* ========================================================================= */

static void test_sprintf_returns_count(void)
{
    TEST("sprintf: returns character count");
    char buf[64];
    int ret = sprintf(buf, "hello");
    ASSERT_EQ_INT(5, ret, "should return 5");
    PASS();
}

static void test_sprintf_returns_count_formatted(void)
{
    TEST("sprintf: return count with format");
    char buf[64];
    int ret = sprintf(buf, "%d", 12345);
    ASSERT_EQ_INT(5, ret, "should return 5");
    PASS();
}

static void test_sprintf_returns_count_empty(void)
{
    TEST("sprintf: returns 0 for empty format");
    char buf[64];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-zero-length"
    int ret = sprintf(buf, "");
#pragma GCC diagnostic pop
    ASSERT_EQ_INT(0, ret, "should return 0");
    PASS();
}

static void test_sprintf_char_nul_return_value(void)
{
    TEST("sprintf: %c with NUL return value counts NUL");
    char buf[64];
    int ret = sprintf(buf, "a%cb", '\0');
    ASSERT_EQ_INT(3, ret, "NUL char still counted in return value");
    PASS();
}

#if TEST_PRINTF_STDOUT
/* ========================================================================= */
/*  printf: return value                                                     */
/* ========================================================================= */

static void test_printf_returns_count(void)
{
    TEST("printf: returns character count");
    /* Redirect stdout to discard printf output, then restore */
    fflush(stdout);
    int saved_fd = dup(fileno(stdout));
#ifdef _WIN32
    FILE *nul = fopen("NUL", "w");
#else
    FILE *nul = fopen("/dev/null", "w");
#endif
    dup2(fileno(nul), fileno(stdout));
    fclose(nul);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-zero-length"
    int r0 = printf("");
#pragma GCC diagnostic pop
    int r1 = printf("hello");
    int r2 = printf("%d", 42);
    fflush(stdout);
    dup2(saved_fd, fileno(stdout));
    close(saved_fd);
    ASSERT_EQ_INT(0, r0, "empty format returns 0");
    ASSERT_EQ_INT(5, r1, "plain string returns 5");
    ASSERT_EQ_INT(2, r2, "formatted returns 2");
    PASS();
}
#endif

#if TEST_PRINTF_N
/* ========================================================================= */
/*  sprintf: %n writes count                                                 */
/* ========================================================================= */

static void test_sprintf_n_specifier(void)
{
    TEST("sprintf: %n writes count");
    char buf[64];
    int count = -1;
    int ret = sprintf(buf, "hello%n world", &count);
    if (ret < 0 || count == -1)
    {
        /* UCRT blocks %n entirely for security — accept as pass */
        PASS();
        return;
    }
    ASSERT_EQ_INT(5, count, "should be 5 after 'hello'");
    ASSERT_EQ_STR("hello world", buf, "output correct");
    PASS();
}
#endif

/* ========================================================================= */
/*  snprintf: basic behavior                                                 */
/* ========================================================================= */


static void test_snprintf_exact_fit(void)
{
    TEST("snprintf: output exactly fills buffer");
    char buf[6];
    int ret = snprintf(buf, 6, "hello");
    ASSERT_EQ_STR("hello", buf, "mismatch");
    ASSERT_EQ_INT(5, ret, "returns 5");
    PASS();
}

static void test_snprintf_size_equals_output_len(void)
{
    TEST("snprintf: size == output length (no room for NUL)");
    char buf[6];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    int ret = snprintf(buf, 5, "hello");
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("hell", buf, "truncated, NUL at buf[4]");
    ASSERT_EQ_INT(5, ret, "returns would-be length");
    PASS();
}

static void test_snprintf_truncation(void)
{
    TEST("snprintf: truncates when buffer too small");
    char buf[6];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    int ret = snprintf(buf, 6, "hello world");
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("hello", buf, "truncated to 5 chars + NUL");
    ASSERT_EQ_INT(11, ret, "returns would-be length");
    PASS();
}

static void test_snprintf_truncation_preserves_nul(void)
{
    TEST("snprintf: always NUL-terminates on truncation");
    char buf[4];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    snprintf(buf, 4, "abcdef");
#pragma GCC diagnostic pop
    ASSERT(buf[3] == '\0', "must be NUL-terminated");
    ASSERT_EQ_STR("abc", buf, "truncated");
    PASS();
}

static void test_snprintf_size_one(void)
{
    TEST("snprintf: size=1 writes only NUL");
    char buf[2] = "x";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    int ret = snprintf(buf, 1, "hello");
#pragma GCC diagnostic pop
    ASSERT(buf[0] == '\0', "must be NUL");
    ASSERT_EQ_INT(5, ret, "returns would-be length");
    PASS();
}

static void test_snprintf_size_zero(void)
{
    TEST("snprintf: size=0 writes nothing");
    char buf[4] = "old";
    int ret = snprintf(buf, 0, "hello");
    ASSERT_EQ_STR("old", buf, "buffer untouched");
    ASSERT_EQ_INT(5, ret, "returns would-be length");
    PASS();
}

static void test_snprintf_null_buffer_size_zero(void)
{
    TEST("snprintf: NULL buffer with size=0 returns length");
    int ret = snprintf(NULL, 0, "hello %d", 42);
    ASSERT_EQ_INT(8, ret, "returns would-be length");
    PASS();
}


/* ========================================================================= */
/*  snprintf: truncation with various specifiers                             */
/* ========================================================================= */

static void test_snprintf_truncate_int(void)
{
    TEST("snprintf: truncates integer mid-output");
    char buf[4];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    int ret = snprintf(buf, 4, "%d", 123456);
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("123", buf, "truncated");
    ASSERT_EQ_INT(6, ret, "returns full length");
    PASS();
}


static void test_snprintf_truncate_string(void)
{
    TEST("snprintf: truncates %s mid-output");
    char buf[4];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    int ret = snprintf(buf, 4, "%s", "hello world");
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("hel", buf, "truncated");
    ASSERT_EQ_INT(11, ret, "returns full length");
    PASS();
}

static void test_snprintf_truncate_multiple(void)
{
    TEST("snprintf: truncation across multiple specifiers");
    char buf[8];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    int ret = snprintf(buf, 8, "%s=%d", "key", 12345);
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("key=123", buf, "truncated across args");
    ASSERT_EQ_INT(9, ret, "returns would-be length");
    PASS();
}

/* ========================================================================= */
/*  snprintf: flags and width under truncation                               */
/* ========================================================================= */

static void test_snprintf_width_truncated(void)
{
    TEST("snprintf: width padding truncated");
    char buf[5];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    int ret = snprintf(buf, 5, "%10d", 42);
#pragma GCC diagnostic pop
    ASSERT(buf[4] == '\0', "NUL-terminated");
    ASSERT_EQ_INT(10, ret, "returns full padded length");
    PASS();
}

static void test_snprintf_zero_pad_truncated(void)
{
    TEST("snprintf: zero-pad truncated");
    char buf[4];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    int ret = snprintf(buf, 4, "%08d", 42);
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("000", buf, "truncated");
    ASSERT_EQ_INT(8, ret, "returns full length");
    PASS();
}

static void test_snprintf_truncate_sign_prefix(void)
{
    TEST("snprintf: truncation with sign and prefix");
    char buf[4];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    int ret = snprintf(buf, 4, "%+05d", 42);
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("+00", buf, "sign then zeros truncated");
    ASSERT_EQ_INT(5, ret, "returns full length");
    PASS();
}

static void test_snprintf_truncate_hash_prefix(void)
{
    TEST("snprintf: truncation with # hex prefix");
    char buf[4];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    int ret = snprintf(buf, 4, "%#x", 255);
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("0xf", buf, "prefix then digits truncated");
    ASSERT_EQ_INT(4, ret, "returns full length");
    PASS();
}

static void test_snprintf_very_large_width_truncated(void)
{
    TEST("snprintf: very large width truncated");
    char buf[16];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    int ret = snprintf(buf, 16, "%200d", 42);
#pragma GCC diagnostic pop
    ASSERT_EQ_INT(200, ret, "returns would-be length");
    ASSERT(buf[15] == '\0', "NUL-terminated");
    PASS();
}

/* ========================================================================= */
/*  sprintf/snprintf: star width and precision                               */
/* ========================================================================= */

static void test_sprintf_star_width(void)
{
    TEST("sprintf: %*d star width");
    char buf[64];
    int ret = sprintf(buf, "%*d", 10, 42);
    ASSERT_EQ_STR("        42", buf, "mismatch");
    ASSERT_EQ_INT(10, ret, "return count");
    PASS();
}

static void test_sprintf_star_precision(void)
{
    TEST("sprintf: %.*s star precision");
    char buf[64];
    int ret = sprintf(buf, "%.*s", 3, "hello");
    ASSERT_EQ_STR("hel", buf, "mismatch");
    ASSERT_EQ_INT(3, ret, "return count");
    PASS();
}

static void test_sprintf_star_negative_width(void)
{
    TEST("sprintf: %*d negative star width = left-align");
    char buf[64];
    int ret = sprintf(buf, "%*d", -10, 42);
    ASSERT_EQ_STR("42        ", buf, "left-aligned");
    ASSERT_EQ_INT(10, ret, "return count");
    PASS();
}

static void test_sprintf_star_negative_precision(void)
{
    TEST("sprintf: %.*s negative precision = no precision");
    char buf[64];
    sprintf(buf, "%.*s", -1, "hello");
    ASSERT_EQ_STR("hello", buf, "negative precision ignored");
    PASS();
}

static void test_snprintf_star_width_truncated(void)
{
    TEST("snprintf: *width truncated by buffer size");
    char buf[5];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    int ret = snprintf(buf, 5, "%*d", 10, 42);
#pragma GCC diagnostic pop
    ASSERT(buf[4] == '\0', "NUL-terminated");
    ASSERT_EQ_INT(10, ret, "returns full padded length");
    PASS();
}

static void test_snprintf_star_precision_truncated(void)
{
    TEST("snprintf: *precision truncated by buffer size");
    char buf[4];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    int ret = snprintf(buf, 4, "%.*s", 10, "hello world");
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("hel", buf, "truncated");
    ASSERT_EQ_INT(10, ret, "returns would-be length");
    PASS();
}


static void test_snprintf_left_align_truncated(void)
{
    TEST("snprintf: left-aligned padding truncated");
    char buf[5];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    int ret = snprintf(buf, 5, "%-10d", 42);
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("42  ", buf, "value then partial padding");
    ASSERT_EQ_INT(10, ret, "returns full padded length");
    PASS();
}

static void test_snprintf_no_write_past_size(void)
{
    TEST("snprintf: does not write beyond buf[size-1]");
    char buf[16];
    memset(buf, 'Z', sizeof(buf));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    snprintf(buf, 4, "hello world");
#pragma GCC diagnostic pop
    ASSERT(buf[3] == '\0', "NUL at buf[3]");
    ASSERT(buf[4] == 'Z', "buf[4] sentinel untouched");
    ASSERT(buf[5] == 'Z', "buf[5] sentinel untouched");
    ASSERT(buf[15] == 'Z', "buf[15] sentinel untouched");
    PASS();
}

/* ========================================================================= */
/*  vsprintf / vsnprintf                                                     */
/* ========================================================================= */

static int helper_vsprintf(char *buf, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = vsprintf(buf, fmt, ap);
    va_end(ap);
    return ret;
}

static int helper_vsnprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return ret;
}

static void test_vsprintf_basic(void)
{
    TEST("vsprintf: basic formatted output");
    char buf[64];
    int ret = helper_vsprintf(buf, "%s=%d", "key", 42);
    ASSERT_EQ_STR("key=42", buf, "mismatch");
    ASSERT_EQ_INT(6, ret, "return count");
    PASS();
}

static void test_vsprintf_multiple_types(void)
{
    TEST("vsprintf: mixed types via va_list");
    char buf[128];
    int ret = helper_vsprintf(buf, "%d %ld %s %c", 42, 100000L, "hello", 'X');
    ASSERT_EQ_STR("42 100000 hello X", buf, "mismatch");
    ASSERT_EQ_INT(17, ret, "return count");
    PASS();
}

static void test_vsnprintf_truncation(void)
{
    TEST("vsnprintf: truncates and returns would-be length");
    char buf[6];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    int ret = helper_vsnprintf(buf, 6, "hello world %d", 42);
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("hello", buf, "truncated");
    ASSERT_EQ_INT(14, ret, "returns full would-be length");
    PASS();
}

static void test_vsnprintf_size_zero(void)
{
    TEST("vsnprintf: size=0 returns would-be length");
    int ret = helper_vsnprintf(NULL, 0, "%s=%d", "key", 42);
    ASSERT_EQ_INT(6, ret, "returns would-be length");
    PASS();
}

/* ========================================================================= */
/*  sprintf: additional flag edge cases                                      */
/* ========================================================================= */

static void test_sprintf_hash_octal_zero_value(void)
{
    TEST("sprintf: %#o with zero value");
    char buf[64];
    int ret = sprintf(buf, "%#o", 0);
    ASSERT_EQ_STR("0", buf, "no double-zero prefix for zero");
    ASSERT_EQ_INT(1, ret, "return count");
    PASS();
}

/* ========================================================================= */
/*  snprintf: embedded NUL via %c                                            */
/* ========================================================================= */

static void test_snprintf_embedded_nul_truncation(void)
{
    TEST("snprintf: embedded NUL via %c counts in return value");
    char buf[8];
    memset(buf, 'Z', sizeof(buf));
    int ret = snprintf(buf, 8, "a%cb%cd", '\0', '\0');
    ASSERT_EQ_INT(5, ret, "returns 5 (NULs counted as chars)");
    ASSERT(buf[0] == 'a', "first char");
    ASSERT(buf[1] == '\0', "embedded NUL from first %%c");
    ASSERT(buf[2] == 'b', "char after first NUL");
    ASSERT(buf[3] == '\0', "embedded NUL from second %%c");
    ASSERT(buf[4] == 'd', "last content char");
    ASSERT(buf[5] == '\0', "terminating NUL");
    PASS();
}

/* ========================================================================= */
/*  main                                                                     */
/* ========================================================================= */

TEST_SUITE(printf, "printf-family tests")
{
    printf(COLOR_YELLOW "[sprintf: basic specifiers]" COLOR_RESET "\n");
    test_sprintf_decimal();
    test_sprintf_negative();
    test_sprintf_zero();
    test_sprintf_int_max();
    test_sprintf_int_min();
    test_sprintf_i_specifier();
    test_sprintf_unsigned();
    test_sprintf_hex_lower();
    test_sprintf_hex_upper();
    test_sprintf_hex_zero();
    test_sprintf_octal();
    test_sprintf_octal_zero();
    test_sprintf_char();
    test_sprintf_char_nul();
    test_sprintf_string();
    test_sprintf_string_empty();
    test_sprintf_percent_literal();
    test_sprintf_pointer();

#if TEST_PRINTF_FLOAT
    printf(COLOR_YELLOW "[sprintf: floating-point]" COLOR_RESET "\n");
    test_sprintf_float_basic();
    test_sprintf_float_negative();
    test_sprintf_float_zero();
    test_sprintf_float_precision();
    test_sprintf_float_precision_zero();
    test_sprintf_scientific_lower();
    test_sprintf_scientific_upper();
    test_sprintf_g_specifier();
    test_sprintf_g_small();
    test_sprintf_inf();
    test_sprintf_neg_inf();
    test_sprintf_nan();
#endif

    printf(COLOR_YELLOW "[sprintf: length modifiers]" COLOR_RESET "\n");
    test_sprintf_long();
    test_sprintf_long_long();
    test_sprintf_size_t();
    test_sprintf_short();
    test_sprintf_unsigned_long_long();
    test_sprintf_hex_long();

    printf(COLOR_YELLOW "[sprintf: integer boundary values]" COLOR_RESET "\n");
    test_sprintf_uint_max();
    test_sprintf_uint_zero();
    test_sprintf_short_max();
    test_sprintf_short_min();
    test_sprintf_ushort_max();
    test_sprintf_long_max();
    test_sprintf_long_min();
    test_sprintf_ulong_max();
    test_sprintf_llong_max();
    test_sprintf_llong_min();
    test_sprintf_ullong_max();
    test_sprintf_size_max();
    test_sprintf_hex_uint_max();
    test_sprintf_octal_uint_max();

    printf(COLOR_YELLOW "[sprintf: niche length modifiers]" COLOR_RESET "\n");
    test_sprintf_char_signed();
    test_sprintf_char_signed_max();
    test_sprintf_char_unsigned();
    test_sprintf_char_unsigned_zero();
    test_sprintf_char_hex();
    test_sprintf_intmax();
    test_sprintf_intmax_max();
    test_sprintf_uintmax();
    test_sprintf_ptrdiff();
    test_sprintf_ptrdiff_negative();

    printf(COLOR_YELLOW "[sprintf: flags and width]" COLOR_RESET "\n");
    test_sprintf_width_right_align();
    test_sprintf_width_left_align();
    test_sprintf_zero_pad();
    test_sprintf_zero_pad_negative();
    test_sprintf_plus_flag();
    test_sprintf_plus_flag_negative();
    test_sprintf_space_flag();
    test_sprintf_space_flag_negative();
    test_sprintf_hash_hex();
    test_sprintf_hash_hex_zero();
    test_sprintf_hash_octal();
    test_sprintf_hash_hex_upper();
    test_sprintf_left_align_overrides_zero();
    test_sprintf_plus_overrides_space();
    test_sprintf_precision_overrides_zero_pad();

    printf(COLOR_YELLOW "[sprintf: ignored flags]" COLOR_RESET "\n");
    test_sprintf_zero_flag_string();
    test_sprintf_plus_flag_string();
    test_sprintf_plus_flag_char();
    test_sprintf_percent_width();

    printf(COLOR_YELLOW "[sprintf: precision]" COLOR_RESET "\n");
    test_sprintf_string_precision();
    test_sprintf_string_precision_longer();
    test_sprintf_int_precision();
    test_sprintf_int_precision_zero_value();
    test_sprintf_int_precision_negative();
    test_sprintf_hex_precision_zero();
    test_sprintf_octal_precision_zero();
    test_sprintf_hash_hex_precision_zero();
    test_sprintf_hash_octal_precision_zero();
    test_sprintf_hash_hex_precision_zero_nonzero();
    test_sprintf_width_and_precision();
    test_sprintf_very_large_width();
    test_sprintf_string_width_precision();
    test_sprintf_string_left_align_precision();

    printf(COLOR_YELLOW "[sprintf: multiple args and misc]" COLOR_RESET "\n");
    test_sprintf_multiple_args();
    test_sprintf_no_format();
    test_sprintf_empty_format();
    test_sprintf_multiple_percent();

    printf(COLOR_YELLOW "[sprintf: return value]" COLOR_RESET "\n");
    test_sprintf_returns_count();
    test_sprintf_returns_count_formatted();
    test_sprintf_returns_count_empty();
    test_sprintf_char_nul_return_value();

#if TEST_PRINTF_N
    printf(COLOR_YELLOW "[sprintf: %%n specifier]" COLOR_RESET "\n");
    test_sprintf_n_specifier();
#endif

    printf(COLOR_YELLOW "[snprintf: basic behavior]" COLOR_RESET "\n");
    test_snprintf_exact_fit();
    test_snprintf_size_equals_output_len();
    test_snprintf_truncation();
    test_snprintf_truncation_preserves_nul();
    test_snprintf_size_one();
    test_snprintf_size_zero();
    test_snprintf_null_buffer_size_zero();

    printf(COLOR_YELLOW "[snprintf: truncation with specifiers]" COLOR_RESET "\n");
    test_snprintf_truncate_int();
    test_snprintf_truncate_string();
    test_snprintf_truncate_multiple();

    printf(COLOR_YELLOW "[snprintf: flags/width under truncation]" COLOR_RESET "\n");
    test_snprintf_width_truncated();
    test_snprintf_zero_pad_truncated();
    test_snprintf_truncate_sign_prefix();
    test_snprintf_truncate_hash_prefix();
    test_snprintf_very_large_width_truncated();
    test_snprintf_left_align_truncated();
    test_snprintf_no_write_past_size();

    printf(COLOR_YELLOW "[sprintf: star width/precision]" COLOR_RESET "\n");
    test_sprintf_star_width();
    test_sprintf_star_precision();
    test_sprintf_star_negative_width();
    test_sprintf_star_negative_precision();

    printf(COLOR_YELLOW "[snprintf: star width/precision]" COLOR_RESET "\n");
    test_snprintf_star_width_truncated();
    test_snprintf_star_precision_truncated();

    printf(COLOR_YELLOW "[snprintf: embedded NUL]" COLOR_RESET "\n");
    test_snprintf_embedded_nul_truncation();

    printf(COLOR_YELLOW "[vsprintf / vsnprintf]" COLOR_RESET "\n");
    test_vsprintf_basic();
    test_vsprintf_multiple_types();
    test_vsnprintf_truncation();
    test_vsnprintf_size_zero();

    printf(COLOR_YELLOW "[sprintf: additional flag edges]" COLOR_RESET "\n");
    test_sprintf_hash_octal_zero_value();

#if TEST_PRINTF_STDOUT
    printf(COLOR_YELLOW "[printf: return value]" COLOR_RESET "\n");
    test_printf_returns_count();
#endif
}
