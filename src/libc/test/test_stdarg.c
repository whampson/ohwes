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
 *         File: src/libc/test/test_stdarg.c
 *      Created: May 23, 2026
 *       Author: Wes Hampson
 *
 * libc stdarg.h function tests
 *
 * Covers: basic variadic extraction, multiple types, type promotion rules,
 *         va_copy independence, zero variadic args, many args, mixed types.
 * =============================================================================
 */

#include <stdarg.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include "framework.h"

/* ========================================================================= */
/*  Helper variadic functions                                                */
/* ========================================================================= */

static int sum_ints(int count, ...)
{
    va_list ap;
    va_start(ap, count);
    int total = 0;
    for (int i = 0; i < count; i++)
        total += va_arg(ap, int);
    va_end(ap);
    return total;
}

static long sum_longs(int count, ...)
{
    va_list ap;
    va_start(ap, count);
    long total = 0;
    for (int i = 0; i < count; i++)
        total += va_arg(ap, long);
    va_end(ap);
    return total;
}

static double sum_doubles(int count, ...)
{
    va_list ap;
    va_start(ap, count);
    double total = 0.0;
    for (int i = 0; i < count; i++)
        total += va_arg(ap, double);
    va_end(ap);
    return total;
}

#if TEST_PRINTF_FLOAT
/* Extracts mixed types: int, long, double, char* in that order */
static void extract_mixed(char *out, size_t size, ...)
{
    va_list ap;
    va_start(ap, size);
    int i = va_arg(ap, int);
    long l = va_arg(ap, long);
    double d = va_arg(ap, double);
    const char *s = va_arg(ap, const char *);
    va_end(ap);
    snprintf(out, size, "%d %ld %.1f %s", i, l, d, s);
}
#else
/* Extracts mixed types: int, long, char* in that order */
static void extract_mixed(char *out, size_t size, ...)
{
    va_list ap;
    va_start(ap, size);
    int i = va_arg(ap, int);
    long l = va_arg(ap, long);
    const char *s = va_arg(ap, const char *);
    va_end(ap);
    snprintf(out, size, "%d %ld %s", i, l, s);
}
#endif

/* Returns the first int from a va_copy'd list */
static int first_via_copy(int count, ...)
{
    va_list ap, ap_copy;
    va_start(ap, count);
    va_copy(ap_copy, ap);
    /* Consume all from original */
    for (int i = 0; i < count; i++)
        (void)va_arg(ap, int);
    va_end(ap);
    /* Read first from copy */
    int first = va_arg(ap_copy, int);
    va_end(ap_copy);
    return first;
}

/* Sums a va_copy'd list independently from the original */
static void sum_both(int count, int *sum_orig, int *sum_copy, ...)
{
    va_list ap, ap_copy;
    va_start(ap, sum_copy);
    va_copy(ap_copy, ap);

    *sum_orig = 0;
    *sum_copy = 0;
    for (int i = 0; i < count; i++)
        *sum_orig += va_arg(ap, int);
    for (int i = 0; i < count; i++)
        *sum_copy += va_arg(ap_copy, int);

    va_end(ap);
    va_end(ap_copy);
}

/* Copies va_list after partial consumption and reads remaining from copy */
static int sum_remaining_via_copy(int count, int skip, ...)
{
    va_list ap, ap_copy;
    va_start(ap, skip);
    /* Consume 'skip' args from original */
    for (int i = 0; i < skip; i++)
        (void)va_arg(ap, int);
    /* Copy after partial consumption */
    va_copy(ap_copy, ap);
    va_end(ap);
    /* Sum remaining from copy */
    int total = 0;
    for (int i = skip; i < count; i++)
        total += va_arg(ap_copy, int);
    va_end(ap_copy);
    return total;
}

/* Returns the n-th pointer argument */
static const char *get_nth_string(int n, ...)
{
    va_list ap;
    va_start(ap, n);
    const char *result = NULL;
    for (int i = 0; i <= n; i++)
        result = va_arg(ap, const char *);
    va_end(ap);
    return result;
}

/* ========================================================================= */
/*  va_start / va_arg / va_end: basic                                        */
/* ========================================================================= */

static void test_va_single_int(void)
{
    TEST("va_arg: single int");
    ASSERT_EQ_INT(42, sum_ints(1, 42), "one arg");
    PASS();
}

static void test_va_multiple_ints(void)
{
    TEST("va_arg: multiple ints");
    ASSERT_EQ_INT(10, sum_ints(4, 1, 2, 3, 4), "sum of 4");
    PASS();
}

static void test_va_zero_args(void)
{
    TEST("va_arg: zero variadic args");
    ASSERT_EQ_INT(0, sum_ints(0), "sum of nothing is 0");
    PASS();
}

static void test_va_negative_ints(void)
{
    TEST("va_arg: negative ints");
    ASSERT_EQ_INT(-6, sum_ints(3, -1, -2, -3), "sum of negatives");
    PASS();
}

static void test_va_many_args(void)
{
    TEST("va_arg: 10 args (register/stack boundary)");
    int result = sum_ints(10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    ASSERT_EQ_INT(55, result, "sum 1..10 = 55");
    PASS();
}

/* ========================================================================= */
/*  va_arg: type handling                                                    */
/* ========================================================================= */

static void test_va_long_type(void)
{
    TEST("va_arg: long type");
    long result = sum_longs(3, 100000L, 200000L, 300000L);
    ASSERT(result == 600000L, "sum of longs");
    PASS();
}

static void test_va_double_type(void)
{
    TEST("va_arg: double type (float promotes to double)");
    double result = sum_doubles(3, 1.5, 2.5, 3.0);
    ASSERT(result > 6.99 && result < 7.01, "sum of doubles ~= 7.0");
    PASS();
}

static void test_va_pointer_type(void)
{
    TEST("va_arg: pointer type");
    const char *r = get_nth_string(2, "alpha", "bravo", "charlie");
    ASSERT_EQ_STR("charlie", r, "3rd string");
    PASS();
}

static void test_va_mixed_types(void)
{
#if TEST_PRINTF_FLOAT
    TEST("va_arg: mixed int, long, double, char*");
    char buf[64];
    extract_mixed(buf, sizeof(buf), 42, 100000L, 3.14, "hello");
    ASSERT_EQ_STR("42 100000 3.1 hello", buf, "mixed extraction");
    PASS();
#else
    TEST("va_arg: mixed int, long, char*");
    char buf[64];
    extract_mixed(buf, sizeof(buf), 42, 100000L, "hello");
    ASSERT_EQ_STR("42 100000 hello", buf, "mixed extraction");
    PASS();
#endif
}

/* ========================================================================= */
/*  Type promotion rules                                                     */
/* ========================================================================= */

static void test_va_char_promotes_to_int(void)
{
    TEST("va_arg: char promotes to int");
    char c = 'A';
    /* char is promoted to int when passed through ... */
    ASSERT_EQ_INT(65, sum_ints(1, c), "char 'A' = 65 as int");
    PASS();
}

static void test_va_short_promotes_to_int(void)
{
    TEST("va_arg: short promotes to int");
    short s = 1000;
    ASSERT_EQ_INT(1000, sum_ints(1, s), "short promotes to int");
    PASS();
}

static void test_va_float_promotes_to_double(void)
{
    TEST("va_arg: float promotes to double");
    float f = 2.5f;
    /* float is promoted to double when passed through ... */
    double result = sum_doubles(1, f);
    ASSERT(result > 2.49 && result < 2.51, "float 2.5 as double");
    PASS();
}

static void test_va_negative_char_promotes(void)
{
    TEST("va_arg: negative char promotes to int");
    char c = -1;
    ASSERT_EQ_INT(-1, sum_ints(1, c), "negative char promotes");
    PASS();
}

static void test_va_negative_short_promotes(void)
{
    TEST("va_arg: negative short promotes to int");
    short s = -1000;
    ASSERT_EQ_INT(-1000, sum_ints(1, s), "negative short promotes");
    PASS();
}

/* ========================================================================= */
/*  va_copy                                                                  */
/* ========================================================================= */

static void test_va_copy_basic(void)
{
    TEST("va_copy: copy reads same first value");
    int first = first_via_copy(3, 10, 20, 30);
    ASSERT_EQ_INT(10, first, "copy sees first arg");
    PASS();
}

static void test_va_copy_independent(void)
{
    TEST("va_copy: original and copy iterate independently");
    int orig, copy;
    sum_both(3, &orig, &copy, 1, 2, 3);
    ASSERT_EQ_INT(6, orig, "original sum");
    ASSERT_EQ_INT(6, copy, "copy sum");
    PASS();
}


static void test_va_copy_after_partial_real(void)
{
    TEST("va_copy: copy after partial consumption");
    int result = sum_remaining_via_copy(5, 2, 10, 20, 30, 40, 50);
    ASSERT_EQ_INT(120, result, "sum of args 3-5 = 30+40+50");
    PASS();
}


/* ========================================================================= */
/*  Edge cases                                                               */
/* ========================================================================= */

static void test_va_pointer_null(void)
{
    TEST("va_arg: NULL pointer extraction");
    const char *r = get_nth_string(0, (const char *)NULL);
    ASSERT(r == NULL, "NULL pointer preserved");
    PASS();
}

static void test_va_int_boundaries(void)
{
    TEST("va_arg: INT_MIN and INT_MAX");
    int result = sum_ints(2, 2147483647, -2147483647);
    ASSERT_EQ_INT(0, result, "INT_MAX + (-INT_MAX) = 0");
    PASS();
}

static void test_va_many_pointers(void)
{
    TEST("va_arg: many pointer args");
    const char *r = get_nth_string(4, "a", "b", "c", "d", "e");
    ASSERT_EQ_STR("e", r, "5th pointer arg");
    PASS();
}

/* ========================================================================= */
/*  Additional type coverage                                                 */
/* ========================================================================= */

static unsigned int sum_uints(int count, ...)
{
    va_list ap;
    va_start(ap, count);
    unsigned int total = 0;
    for (int i = 0; i < count; i++)
        total += va_arg(ap, unsigned int);
    va_end(ap);
    return total;
}

static unsigned long long sum_ullongs(int count, ...)
{
    va_list ap;
    va_start(ap, count);
    unsigned long long total = 0;
    for (int i = 0; i < count; i++)
        total += va_arg(ap, unsigned long long);
    va_end(ap);
    return total;
}

static size_t sum_sizes(int count, ...)
{
    va_list ap;
    va_start(ap, count);
    size_t total = 0;
    for (int i = 0; i < count; i++)
        total += va_arg(ap, size_t);
    va_end(ap);
    return total;
}

static long double sum_long_doubles(int count, ...)
{
    va_list ap;
    va_start(ap, count);
    long double total = 0.0L;
    for (int i = 0; i < count; i++)
        total += va_arg(ap, long double);
    va_end(ap);
    return total;
}

static void test_va_unsigned_int(void)
{
    TEST("va_arg: unsigned int type");
    unsigned int result = sum_uints(3, 100u, 200u, 300u);
    ASSERT(result == 600u, "sum of unsigned ints");
    PASS();
}

static void test_va_unsigned_int_max(void)
{
    TEST("va_arg: UINT_MAX unsigned int");
    unsigned int result = sum_uints(1, UINT_MAX);
    ASSERT(result == UINT_MAX, "UINT_MAX preserved");
    PASS();
}

static void test_va_unsigned_long_long(void)
{
    TEST("va_arg: unsigned long long type");
    unsigned long long result = sum_ullongs(3, 1000000000ULL, 2000000000ULL, 3000000000ULL);
    ASSERT(result == 6000000000ULL, "sum of unsigned long longs");
    PASS();
}


static void test_va_size_t(void)
{
    TEST("va_arg: size_t type");
    size_t result = sum_sizes(3, (size_t)10, (size_t)20, (size_t)30);
    ASSERT(result == 60, "sum of size_t values");
    PASS();
}


static void test_va_long_double(void)
{
    TEST("va_arg: long double type");
    long double result = sum_long_doubles(3, 1.5L, 2.5L, 3.0L);
    ASSERT(result > 6.99L && result < 7.01L, "sum of long doubles ~= 7.0");
    PASS();
}


/* ========================================================================= */
/*  Suite runner                                                             */
/* ========================================================================= */

TEST_SUITE(stdarg, "stdarg.h tests")
{
    printf(COLOR_YELLOW "[va_start / va_arg / va_end]" COLOR_RESET "\n");
    test_va_single_int();
    test_va_multiple_ints();
    test_va_zero_args();
    test_va_negative_ints();
    test_va_many_args();

    printf(COLOR_YELLOW "[va_arg: type handling]" COLOR_RESET "\n");
    test_va_long_type();
    test_va_double_type();
    test_va_pointer_type();
    test_va_mixed_types();

    printf(COLOR_YELLOW "[type promotion rules]" COLOR_RESET "\n");
    test_va_char_promotes_to_int();
    test_va_short_promotes_to_int();
    test_va_float_promotes_to_double();
    test_va_negative_char_promotes();
    test_va_negative_short_promotes();

    printf(COLOR_YELLOW "[va_copy]" COLOR_RESET "\n");
    test_va_copy_basic();
    test_va_copy_independent();
    test_va_copy_after_partial_real();

    printf(COLOR_YELLOW "[edge cases]" COLOR_RESET "\n");
    test_va_pointer_null();
    test_va_int_boundaries();
    test_va_many_pointers();

    printf(COLOR_YELLOW "[additional type coverage]" COLOR_RESET "\n");
    test_va_unsigned_int();
    test_va_unsigned_int_max();
    test_va_unsigned_long_long();
    test_va_size_t();
    test_va_long_double();
}
