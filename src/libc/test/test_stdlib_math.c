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
 *         File: src/libc/test/test_stdlib_math.c
 *      Created: May 24, 2026
 *       Author: Wes Hampson
 *
 * libc stdlib.h integer math function tests
 *
 * Covers: positive/negative/zero inputs, boundary values (INT_MIN, etc.),
 *         quotient and remainder correctness, identity properties.
 * =============================================================================
 */

#include <stdlib.h>
#include <limits.h>
#include <inttypes.h>
#include "framework.h"

#if TEST_STDLIB_MATH

/* ========================================================================= */
/*  abs                                                                      */
/* ========================================================================= */


static void test_abs_negative(void)
{
    TEST("abs: negative becomes positive");
    ASSERT_EQ_INT(42, abs(-42), "abs(-42)");
    PASS();
}

static void test_abs_zero(void)
{
    TEST("abs: zero");
    ASSERT_EQ_INT(0, abs(0), "abs(0)");
    PASS();
}

static void test_abs_one(void)
{
    TEST("abs: one and negative one");
    ASSERT_EQ_INT(1, abs(1), "abs(1)");
    ASSERT_EQ_INT(1, abs(-1), "abs(-1)");
    PASS();
}

static void test_abs_int_max(void)
{
    TEST("abs: INT_MAX unchanged");
    ASSERT_EQ_INT(INT_MAX, abs(INT_MAX), "abs(INT_MAX)");
    PASS();
}

static void test_abs_int_min_plus_one(void)
{
    TEST("abs: INT_MIN+1 (safe boundary)");
    ASSERT_EQ_INT(INT_MAX, abs(INT_MIN + 1), "abs(INT_MIN+1) == INT_MAX");
    PASS();
}

/* NOTE: abs(INT_MIN) is undefined behavior on two's complement overflow.
 * We intentionally do not test it. */

/* ========================================================================= */
/*  labs                                                                     */
/* ========================================================================= */


static void test_labs_negative(void)
{
    TEST("labs: negative becomes positive");
    ASSERT(labs(-100000L) == 100000L, "labs(-100000)");
    PASS();
}

static void test_labs_zero(void)
{
    TEST("labs: zero");
    ASSERT(labs(0L) == 0L, "labs(0)");
    PASS();
}

static void test_labs_long_max(void)
{
    TEST("labs: LONG_MAX unchanged");
    ASSERT(labs(LONG_MAX) == LONG_MAX, "labs(LONG_MAX)");
    PASS();
}

static void test_labs_long_min_plus_one(void)
{
    TEST("labs: LONG_MIN+1 (safe boundary)");
    ASSERT(labs(LONG_MIN + 1) == LONG_MAX, "labs(LONG_MIN+1) == LONG_MAX");
    PASS();
}

/* ========================================================================= */
/*  llabs                                                                    */
/* ========================================================================= */


static void test_llabs_negative(void)
{
    TEST("llabs: negative becomes positive");
    ASSERT(llabs(-1000000000LL) == 1000000000LL, "llabs negative");
    PASS();
}

static void test_llabs_zero(void)
{
    TEST("llabs: zero");
    ASSERT(llabs(0LL) == 0LL, "llabs(0)");
    PASS();
}

static void test_llabs_llong_max(void)
{
    TEST("llabs: LLONG_MAX unchanged");
    ASSERT(llabs(LLONG_MAX) == LLONG_MAX, "llabs(LLONG_MAX)");
    PASS();
}

static void test_llabs_llong_min_plus_one(void)
{
    TEST("llabs: LLONG_MIN+1 (safe boundary)");
    ASSERT(llabs(LLONG_MIN + 1) == LLONG_MAX, "llabs(LLONG_MIN+1) == LLONG_MAX");
    PASS();
}

/* ========================================================================= */
/*  div                                                                      */
/* ========================================================================= */

static void test_div_basic(void)
{
    TEST("div: 10 / 3 = quot 3, rem 1");
    div_t r = div(10, 3);
    ASSERT_EQ_INT(3, r.quot, "quotient");
    ASSERT_EQ_INT(1, r.rem, "remainder");
    PASS();
}

static void test_div_exact(void)
{
    TEST("div: exact division, remainder 0");
    div_t r = div(12, 4);
    ASSERT_EQ_INT(3, r.quot, "quotient");
    ASSERT_EQ_INT(0, r.rem, "remainder");
    PASS();
}

static void test_div_negative_numerator(void)
{
    TEST("div: negative numerator");
    div_t r = div(-10, 3);
    ASSERT_EQ_INT(-3, r.quot, "quotient");
    ASSERT_EQ_INT(-1, r.rem, "remainder");
    PASS();
}

static void test_div_negative_denominator(void)
{
    TEST("div: negative denominator");
    div_t r = div(10, -3);
    ASSERT_EQ_INT(-3, r.quot, "quotient");
    ASSERT_EQ_INT(1, r.rem, "remainder");
    PASS();
}

static void test_div_both_negative(void)
{
    TEST("div: both negative");
    div_t r = div(-10, -3);
    ASSERT_EQ_INT(3, r.quot, "quotient");
    ASSERT_EQ_INT(-1, r.rem, "remainder");
    PASS();
}

static void test_div_zero_numerator(void)
{
    TEST("div: 0 / n = quot 0, rem 0");
    div_t r = div(0, 5);
    ASSERT_EQ_INT(0, r.quot, "quotient");
    ASSERT_EQ_INT(0, r.rem, "remainder");
    PASS();
}

static void test_div_one_denominator(void)
{
    TEST("div: n / 1 = quot n, rem 0");
    div_t r = div(42, 1);
    ASSERT_EQ_INT(42, r.quot, "quotient");
    ASSERT_EQ_INT(0, r.rem, "remainder");
    PASS();
}


static void test_div_identity_negative(void)
{
    TEST("div: identity holds for negative inputs");
    div_t r = div(-17, 5);
    ASSERT_EQ_INT(-17, r.quot * 5 + r.rem, "identity holds");
    r = div(17, -5);
    ASSERT_EQ_INT(17, r.quot * (-5) + r.rem, "identity holds");
    r = div(-17, -5);
    ASSERT_EQ_INT(-17, r.quot * (-5) + r.rem, "identity holds");
    PASS();
}

/* ========================================================================= */
/*  ldiv                                                                     */
/* ========================================================================= */

static void test_ldiv_basic(void)
{
    TEST("ldiv: basic division");
    ldiv_t r = ldiv(100000L, 7L);
    ASSERT(r.quot == 14285L, "quotient");
    ASSERT(r.rem == 5L, "remainder");
    PASS();
}

static void test_ldiv_negative(void)
{
    TEST("ldiv: negative numerator");
    ldiv_t r = ldiv(-100000L, 7L);
    ASSERT(r.quot == -14285L, "quotient");
    ASSERT(r.rem == -5L, "remainder");
    PASS();
}


static void test_ldiv_exact(void)
{
    TEST("ldiv: exact division");
    ldiv_t r = ldiv(12L, 4L);
    ASSERT(r.quot == 3L && r.rem == 0L, "exact");
    PASS();
}

static void test_ldiv_negative_denom(void)
{
    TEST("ldiv: negative denominator");
    ldiv_t r = ldiv(10L, -3L);
    ASSERT(r.quot == -3L && r.rem == 1L, "neg denom");
    PASS();
}

static void test_ldiv_both_negative(void)
{
    TEST("ldiv: both negative");
    ldiv_t r = ldiv(-10L, -3L);
    ASSERT(r.quot == 3L && r.rem == -1L, "both neg");
    PASS();
}

static void test_ldiv_zero_numerator(void)
{
    TEST("ldiv: 0 / n");
    ldiv_t r = ldiv(0L, 5L);
    ASSERT(r.quot == 0L && r.rem == 0L, "zero num");
    PASS();
}

static void test_ldiv_one_denominator(void)
{
    TEST("ldiv: n / 1 = quot n, rem 0");
    ldiv_t r = ldiv(42L, 1L);
    ASSERT(r.quot == 42L && r.rem == 0L, "n / 1");
    PASS();
}

static void test_ldiv_identity_negative(void)
{
    TEST("ldiv: identity holds for negative inputs");
    ldiv_t r = ldiv(-17L, 5L);
    ASSERT(r.quot * 5L + r.rem == -17L, "identity holds");
    r = ldiv(17L, -5L);
    ASSERT(r.quot * (-5L) + r.rem == 17L, "identity holds");
    r = ldiv(-17L, -5L);
    ASSERT(r.quot * (-5L) + r.rem == -17L, "identity holds");
    PASS();
}

static void test_ldiv_small_negative_denominator(void)
{
    TEST("ldiv: 3 / -5 truncates toward zero");
    ldiv_t r = ldiv(3L, -5L);
    ASSERT(r.quot == 0L && r.rem == 3L, "small neg denom");
    PASS();
}

static void test_ldiv_negative_exact(void)
{
    TEST("ldiv: -12 / 4 exact negative division");
    ldiv_t r = ldiv(-12L, 4L);
    ASSERT(r.quot == -3L && r.rem == 0L, "neg exact");
    PASS();
}

/* ========================================================================= */
/*  lldiv                                                                    */
/* ========================================================================= */

static void test_lldiv_basic(void)
{
    TEST("lldiv: basic division");
    lldiv_t r = lldiv(10000000000LL, 7LL);
    ASSERT(r.quot == 1428571428LL, "quotient");
    ASSERT(r.rem == 4LL, "remainder");
    PASS();
}

static void test_lldiv_negative(void)
{
    TEST("lldiv: negative numerator");
    lldiv_t r = lldiv(-10000000000LL, 7LL);
    ASSERT(r.quot == -1428571428LL, "quotient");
    ASSERT(r.rem == -4LL, "remainder");
    PASS();
}


static void test_lldiv_exact(void)
{
    TEST("lldiv: exact division");
    lldiv_t r = lldiv(12LL, 4LL);
    ASSERT(r.quot == 3LL && r.rem == 0LL, "exact");
    PASS();
}

static void test_lldiv_negative_denom(void)
{
    TEST("lldiv: negative denominator");
    lldiv_t r = lldiv(10LL, -3LL);
    ASSERT(r.quot == -3LL && r.rem == 1LL, "neg denom");
    PASS();
}

static void test_lldiv_both_negative(void)
{
    TEST("lldiv: both negative");
    lldiv_t r = lldiv(-10LL, -3LL);
    ASSERT(r.quot == 3LL && r.rem == -1LL, "both neg");
    PASS();
}

static void test_lldiv_zero_numerator(void)
{
    TEST("lldiv: 0 / n");
    lldiv_t r = lldiv(0LL, 5LL);
    ASSERT(r.quot == 0LL && r.rem == 0LL, "zero num");
    PASS();
}

static void test_lldiv_one_denominator(void)
{
    TEST("lldiv: n / 1 = quot n, rem 0");
    lldiv_t r = lldiv(42LL, 1LL);
    ASSERT(r.quot == 42LL && r.rem == 0LL, "n / 1");
    PASS();
}

static void test_lldiv_identity_negative(void)
{
    TEST("lldiv: identity holds for negative inputs");
    lldiv_t r = lldiv(-17LL, 5LL);
    ASSERT(r.quot * 5LL + r.rem == -17LL, "identity holds");
    r = lldiv(17LL, -5LL);
    ASSERT(r.quot * (-5LL) + r.rem == 17LL, "identity holds");
    r = lldiv(-17LL, -5LL);
    ASSERT(r.quot * (-5LL) + r.rem == -17LL, "identity holds");
    PASS();
}

static void test_lldiv_small_negative_denominator(void)
{
    TEST("lldiv: 3 / -5 truncates toward zero");
    lldiv_t r = lldiv(3LL, -5LL);
    ASSERT(r.quot == 0LL && r.rem == 3LL, "small neg denom");
    PASS();
}

static void test_lldiv_negative_exact(void)
{
    TEST("lldiv: -12 / 4 exact negative division");
    lldiv_t r = lldiv(-12LL, 4LL);
    ASSERT(r.quot == -3LL && r.rem == 0LL, "neg exact");
    PASS();
}

/* ========================================================================= */
/*  div/ldiv/lldiv: small numerator (|numer| < |denom|)                      */
/* ========================================================================= */

static void test_div_small_numerator(void)
{
    TEST("div: |numer| < |denom| truncates to 0");
    div_t r = div(3, 5);
    ASSERT_EQ_INT(0, r.quot, "quotient");
    ASSERT_EQ_INT(3, r.rem, "remainder");
    PASS();
}

static void test_div_small_negative_numerator(void)
{
    TEST("div: -3 / 5 truncates toward zero");
    div_t r = div(-3, 5);
    ASSERT_EQ_INT(0, r.quot, "quotient");
    ASSERT_EQ_INT(-3, r.rem, "remainder");
    PASS();
}

static void test_div_small_negative_denominator(void)
{
    TEST("div: 3 / -5 truncates toward zero");
    div_t r = div(3, -5);
    ASSERT_EQ_INT(0, r.quot, "quotient");
    ASSERT_EQ_INT(3, r.rem, "remainder");
    PASS();
}

static void test_div_denom_neg_one(void)
{
    TEST("div: (INT_MIN+1) / -1");
    div_t r = div(INT_MIN + 1, -1);
    ASSERT_EQ_INT(INT_MAX, r.quot, "quotient");
    ASSERT_EQ_INT(0, r.rem, "remainder");
    PASS();
}

static void test_div_negative_exact(void)
{
    TEST("div: -12 / 4 exact negative division");
    div_t r = div(-12, 4);
    ASSERT_EQ_INT(-3, r.quot, "quotient");
    ASSERT_EQ_INT(0, r.rem, "remainder");
    PASS();
}

static void test_ldiv_small_numerator(void)
{
    TEST("ldiv: |numer| < |denom| truncates to 0");
    ldiv_t r = ldiv(3L, 5L);
    ASSERT(r.quot == 0L && r.rem == 3L, "small numerator");
    PASS();
}

static void test_ldiv_small_negative_numerator(void)
{
    TEST("ldiv: -3 / 5 truncates toward zero");
    ldiv_t r = ldiv(-3L, 5L);
    ASSERT(r.quot == 0L && r.rem == -3L, "neg small numerator");
    PASS();
}

static void test_ldiv_denom_neg_one(void)
{
    TEST("ldiv: (LONG_MIN+1) / -1");
    ldiv_t r = ldiv(LONG_MIN + 1, -1L);
    ASSERT(r.quot == LONG_MAX, "quotient");
    ASSERT(r.rem == 0L, "remainder");
    PASS();
}

static void test_ldiv_long_max(void)
{
    TEST("ldiv: LONG_MAX / 1");
    ldiv_t r = ldiv(LONG_MAX, 1L);
    ASSERT(r.quot == LONG_MAX && r.rem == 0L, "LONG_MAX / 1");
    PASS();
}

static void test_ldiv_long_max_by_two(void)
{
    TEST("ldiv: LONG_MAX / 2");
    ldiv_t r = ldiv(LONG_MAX, 2L);
    ASSERT(r.quot == LONG_MAX / 2 && r.rem == 1L, "LONG_MAX / 2");
    PASS();
}

static void test_lldiv_small_numerator(void)
{
    TEST("lldiv: |numer| < |denom| truncates to 0");
    lldiv_t r = lldiv(3LL, 5LL);
    ASSERT(r.quot == 0LL && r.rem == 3LL, "small numerator");
    PASS();
}

static void test_lldiv_small_negative_numerator(void)
{
    TEST("lldiv: -3 / 5 truncates toward zero");
    lldiv_t r = lldiv(-3LL, 5LL);
    ASSERT(r.quot == 0LL && r.rem == -3LL, "neg small numerator");
    PASS();
}

static void test_lldiv_denom_neg_one(void)
{
    TEST("lldiv: (LLONG_MIN+1) / -1");
    lldiv_t r = lldiv(LLONG_MIN + 1, -1LL);
    ASSERT(r.quot == LLONG_MAX, "quotient");
    ASSERT(r.rem == 0LL, "remainder");
    PASS();
}

static void test_lldiv_llong_max(void)
{
    TEST("lldiv: LLONG_MAX / 1");
    lldiv_t r = lldiv(LLONG_MAX, 1LL);
    ASSERT(r.quot == LLONG_MAX && r.rem == 0LL, "LLONG_MAX / 1");
    PASS();
}

static void test_lldiv_llong_max_by_two(void)
{
    TEST("lldiv: LLONG_MAX / 2");
    lldiv_t r = lldiv(LLONG_MAX, 2LL);
    ASSERT(r.quot == LLONG_MAX / 2 && r.rem == 1LL, "LLONG_MAX / 2");
    PASS();
}

/* ========================================================================= */
/*  imaxabs                                                                  */
/* ========================================================================= */

static void test_imaxabs_negative(void)
{
    TEST("imaxabs: negative becomes positive");
    ASSERT(imaxabs((intmax_t)-42) == 42, "imaxabs(-42)");
    PASS();
}

static void test_imaxabs_zero(void)
{
    TEST("imaxabs: zero returns zero");
    ASSERT(imaxabs((intmax_t)0) == 0, "imaxabs(0)");
    PASS();
}

static void test_imaxabs_intmax_max(void)
{
    TEST("imaxabs: INTMAX_MAX unchanged");
    ASSERT(imaxabs(INTMAX_MAX) == INTMAX_MAX, "imaxabs(INTMAX_MAX)");
    PASS();
}

/* ========================================================================= */
/*  imaxdiv                                                                  */
/* ========================================================================= */

static void test_imaxdiv_basic(void)
{
    TEST("imaxdiv: basic quotient and remainder");
    imaxdiv_t r = imaxdiv((intmax_t)17, (intmax_t)5);
    ASSERT(r.quot == 3 && r.rem == 2, "17/5 = 3 rem 2");
    PASS();
}

static void test_imaxdiv_negative_numerator(void)
{
    TEST("imaxdiv: negative numerator");
    imaxdiv_t r = imaxdiv((intmax_t)-17, (intmax_t)5);
    ASSERT(r.quot == -3 && r.rem == -2, "-17/5 = -3 rem -2");
    PASS();
}

static void test_imaxdiv_exact(void)
{
    TEST("imaxdiv: exact division");
    imaxdiv_t r = imaxdiv((intmax_t)20, (intmax_t)5);
    ASSERT(r.quot == 4 && r.rem == 0, "exact");
    PASS();
}

static void test_imaxdiv_identity(void)
{
    TEST("imaxdiv: quot * denom + rem == numer");
    imaxdiv_t r = imaxdiv((intmax_t)-17, (intmax_t)5);
    ASSERT(r.quot * 5 + r.rem == -17, "identity holds");
    PASS();
}

static void test_imaxdiv_large_values(void)
{
    TEST("imaxdiv: large intmax_t values");
    imaxdiv_t r = imaxdiv(INTMAX_MAX, (intmax_t)2);
    ASSERT(r.quot == INTMAX_MAX / 2 && r.rem == 1, "INTMAX_MAX / 2");
    PASS();
}

/* ========================================================================= */
/*  Suite runner                                                             */
/* ========================================================================= */

TEST_SUITE(stdlib_math, "stdlib.h integer math tests")
{
    printf(COLOR_YELLOW "[abs]" COLOR_RESET "\n");
    test_abs_negative();
    test_abs_zero();
    test_abs_one();
    test_abs_int_max();
    test_abs_int_min_plus_one();

    printf(COLOR_YELLOW "[labs]" COLOR_RESET "\n");
    test_labs_negative();
    test_labs_zero();
    test_labs_long_max();
    test_labs_long_min_plus_one();

    printf(COLOR_YELLOW "[llabs]" COLOR_RESET "\n");
    test_llabs_negative();
    test_llabs_zero();
    test_llabs_llong_max();
    test_llabs_llong_min_plus_one();

    printf(COLOR_YELLOW "[div]" COLOR_RESET "\n");
    test_div_basic();
    test_div_exact();
    test_div_negative_numerator();
    test_div_negative_denominator();
    test_div_both_negative();
    test_div_zero_numerator();
    test_div_one_denominator();
    test_div_identity_negative();
    test_div_small_numerator();
    test_div_small_negative_numerator();
    test_div_small_negative_denominator();
    test_div_denom_neg_one();
    test_div_negative_exact();

    printf(COLOR_YELLOW "[ldiv]" COLOR_RESET "\n");
    test_ldiv_basic();
    test_ldiv_negative();
    test_ldiv_exact();
    test_ldiv_negative_denom();
    test_ldiv_both_negative();
    test_ldiv_zero_numerator();
    test_ldiv_one_denominator();
    test_ldiv_identity_negative();
    test_ldiv_small_numerator();
    test_ldiv_small_negative_numerator();
    test_ldiv_small_negative_denominator();
    test_ldiv_negative_exact();
    test_ldiv_denom_neg_one();
    test_ldiv_long_max();
    test_ldiv_long_max_by_two();

    printf(COLOR_YELLOW "[lldiv]" COLOR_RESET "\n");
    test_lldiv_basic();
    test_lldiv_negative();
    test_lldiv_exact();
    test_lldiv_negative_denom();
    test_lldiv_both_negative();
    test_lldiv_zero_numerator();
    test_lldiv_one_denominator();
    test_lldiv_identity_negative();
    test_lldiv_small_numerator();
    test_lldiv_small_negative_numerator();
    test_lldiv_small_negative_denominator();
    test_lldiv_negative_exact();
    test_lldiv_denom_neg_one();
    test_lldiv_llong_max();
    test_lldiv_llong_max_by_two();

    printf(COLOR_YELLOW "[imaxabs]" COLOR_RESET "\n");
    test_imaxabs_negative();
    test_imaxabs_zero();
    test_imaxabs_intmax_max();

    printf(COLOR_YELLOW "[imaxdiv]" COLOR_RESET "\n");
    test_imaxdiv_basic();
    test_imaxdiv_negative_numerator();
    test_imaxdiv_exact();
    test_imaxdiv_identity();
    test_imaxdiv_large_values();
}

#endif  // TEST_STDLIB_MATH.
