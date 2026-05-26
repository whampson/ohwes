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
 *         File: src/libc/test/main.c
 *      Created: May 22, 2026
 *       Author: Wes Hampson
 *
 * libc test runner.
 * =============================================================================
 */

#include "framework.h"

DECLARE_TEST_SUITE(string);
DECLARE_TEST_SUITE(printf);
DECLARE_TEST_SUITE(strtol);
DECLARE_TEST_SUITE(ctype);
DECLARE_TEST_SUITE(stdarg);
DECLARE_TEST_SUITE(stdtypes);
DECLARE_TEST_SUITE(stdlib_math);

// int main(void)
int test_libc(void)
{
    printf(COLOR_BOLD "\n=== libc Test Suite ===" COLOR_RESET "\n");

    run_string_tests();
    run_printf_tests();
    run_strtol_tests();
    run_ctype_tests();
    run_stdarg_tests();
    run_stdtypes_tests();
#if TEST_STDLIB_MATH
    run_stdlib_math_tests();
#endif

    if (tests_failed > 0)
        printf("\n" COLOR_BOLD "=== " COLOR_GREEN "%d/%d passed" COLOR_RESET COLOR_BOLD ", " COLOR_RED "%d FAILED" COLOR_RESET COLOR_BOLD " ===" COLOR_RESET "\n",
               tests_passed, tests_run, tests_failed);
    else
        printf("\n" COLOR_BOLD "=== " COLOR_GREEN "%d/%d passed" COLOR_RESET COLOR_BOLD " ===" COLOR_RESET "\n",
               tests_passed, tests_run);

    print_failure_summary();
    printf("\n");

    return tests_failed > 0 ? 1 : 0;
}
