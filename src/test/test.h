/* =============================================================================
 * Copyright (C) 2020-2025 Wes Hampson. All Rights Reserved.
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
 *         File: test/test.h
 *      Created: June 16, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __TEST_H
#define __TEST_H

#define TEST_PASSED 0
#define TEST_FAILED 1

#include <ohwes.h>
#include <panic.h>
#include <stdio.h>
#include <errno.h>

#define _FAIL_TEST(fn,...)                                                      \
do {                                                                            \
    errno = TEST_FAILED;                                                        \
    puts("\n*** TEST FAILED ***\n");                                            \
    puts(__FILE__ ":" STRINGIFY_LITERAL(__LINE__) "\n");                        \
    puts("\t" fn "(" #__VA_ARGS__ ")");                                         \
    exit(TEST_FAILED);                                                          \
} while (0)

#define VERIFY_IS_TRUE(x)                                                       \
do {                                                                            \
    if (!(x)) {                                                                 \
        _FAIL_TEST("VERIFY_IS_TRUE", x);                                        \
    }                                                                           \
} while (0)

#define VERIFY_IS_FALSE(x)                                                      \
do {                                                                            \
    if (x) {                                                                    \
        _FAIL_TEST("VERIFY_IS_FALSE", x);                                       \
    }                                                                           \
} while (0)

#define VERIFY_IS_ZERO(x)                                                       \
do {                                                                            \
    if ((x) != 0) {                                                             \
        _FAIL_TEST("VERIFY_IS_ZERO", x);                                        \
    }                                                                           \
} while (0)

#define VERIFY_IS_NOT_ZERO(x)                                                   \
do {                                                                            \
    if ((x) == 0) {                                                             \
        _FAIL_TEST("VERIFY_IS_NOT_ZERO", x);                                    \
    }                                                                           \
} while (0)

#define VERIFY_IS_NULL(x)                                                       \
do {                                                                            \
    if ((x) != NULL) {                                                          \
        _FAIL_TEST("VERIFY_IS_NULL", x);                                        \
    }                                                                           \
} while (0)

#define VERIFY_IS_NOT_NULL(x)                                                   \
do {                                                                            \
    if ((x) == NULL) {                                                          \
        _FAIL_TEST("VERIFY_IS_NOT_NULL", x);                                    \
    }                                                                           \
} while (0)

#define VERIFY_ARE_EQUAL(x,y)                                                   \
do {                                                                            \
    if ((x) != (y)) {                                                           \
        _FAIL_TEST("VERIFY_ARE_EQUAL", x, y);                                   \
    }                                                                           \
} while (0)

#define VERIFY_ARE_NOT_EQUAL(x,y)                                               \
do {                                                                            \
    if ((x) == (y)) {                                                           \
        _FAIL_TEST("VERIFY_ARE_NOT_EQUAL", x, y);                               \
    }                                                                           \
} while (0)

#endif // __TEST_H
