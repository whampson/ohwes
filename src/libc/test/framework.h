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
 *         File: src/libc/test/framework.h
 *      Created: May 22, 2026
 *       Author: Wes Hampson
 *
 * Minimal test framework.
 * =============================================================================
 */

#ifndef __TEST_FRAMEWORK_H
#define __TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifndef FAIL_FAST
#define FAIL_FAST           0   /* assert on first failure */
#endif

#ifndef TEST_PRINTF_FLOAT
#define TEST_PRINTF_FLOAT   0   /* enable floating-point specifier tests */
#endif

#ifndef TEST_PRINTF_N
#define TEST_PRINTF_N       0   /* enable %n specifier tests */
#endif

#ifndef TEST_PRINTF_STDOUT
#define TEST_PRINTF_STDOUT  0   /* enable tests for printf itself, otherwise only sprintf/snprintf tested */
#endif

#ifndef TEST_STDLIB_MATH
#define TEST_STDLIB_MATH    0   /* enable abs/div tests*/
#endif

#ifndef TEST_STRCHR
#define TEST_STRCHR         0   /* enable strchr/strrchr tests */
#endif

#ifndef TEST_STRSTR
#define TEST_STRSTR         0   /* enable strstr tests */
#endif

#ifndef TEST_STRSPAN
#define TEST_STRSPAN        0   /* enable strspan/strcspan tests */
#endif

#ifndef TEST_STRPBRK
#define TEST_STRPBRK        0   /* enable strpbrk tests */
#endif

#ifndef TEST_MEMCHR
#define TEST_MEMCHR         0   /* enable memchr tests */
#endif

#ifndef TEST_ATOI
#define TEST_ATOI           0   /* test atoi/atol/atoll */
#endif

#ifndef TEST_STRTOIMAX
#define TEST_STRTOIMAX      0   /* test strtoimax/strtoumax */
#endif

/* -- Colors ---------------------------------------------------------------- */

#define COLOR_RESET         "\033[0m"
#define COLOR_GREEN         "\033[32m"
#define COLOR_RED           "\033[31m"
#define COLOR_YELLOW        "\033[33m"
#define COLOR_CYAN          "\033[36m"
#define COLOR_BOLD          "\033[1m"
#define COLOR_NOBOLD        "\033[22m"

/* -- Failure log (static, no dynamic allocation) --------------------------- */

#define MAX_FAIL            256
#define MAX_FAIL_MSG_LEN    128
#define MAX_FAIL_FILE_LEN   32

struct failure_record {
    char msg[MAX_FAIL_MSG_LEN];
    char file[MAX_FAIL_FILE_LEN];
    int  line;
} ;

extern struct failure_record failure_log[MAX_FAIL];

void record_failure(const char *msg, const char *file,int line);
void print_failure_summary(void);

/* -- Counters -------------------------------------------------------------- */

extern int tests_run;
extern int tests_passed;
extern int tests_failed;

/* -- Test macros ----------------------------------------------------------- */

#if FAIL_FAST
#define FAIL_FAST_CHECK(msg) assert(0 && msg)
#else
#define FAIL_FAST_CHECK(msg) ((void)0)
#endif

#define TEST(name) \
    do { \
        tests_run++; \
        printf("  %-55s ", name); \
    } while (0)

#define PASS() \
    do { \
        tests_passed++; \
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n"); \
    } while (0)

#define FAIL(msg) \
    do { \
        record_failure(msg, __FILE__, __LINE__); \
        printf(COLOR_BOLD COLOR_RED "[FAIL]" COLOR_NOBOLD " %s" COLOR_RESET "\n", msg); \
        FAIL_FAST_CHECK(msg); \
    } while (0)

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { FAIL(msg); return; } \
    } while (0)

#define ASSERT_EQ_INT(expected, actual, msg) \
    ASSERT((expected) == (actual), msg)

#define ASSERT_EQ_STR(expected, actual, msg) \
    ASSERT(strcmp((expected), (actual)) == 0, msg)

/* -- Suite macros ---------------------------------------------------------- */

#define DECLARE_TEST_SUITE(name) \
    extern void run_##name##_tests(void)

/**
 * Define a test suite with a banner header.
 *
 * Usage:
 * ```
 *   TEST_SUITE(string, "string.h tests")
 *   {
 *       test_strlen_empty();
 *       ...
 *   }
 * ```
 */
#define TEST_SUITE(name, label) \
    static void run_##name##_tests_body(void); \
    void run_##name##_tests(void) { \
        printf("\n" COLOR_BOLD COLOR_CYAN "--- " label " ---" COLOR_RESET "\n\n"); \
        run_##name##_tests_body(); \
    } \
    static void run_##name##_tests_body(void)

#endif /* __TEST_FRAMEWORK_H */
