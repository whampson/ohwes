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

#define STRINGIFY(x)            # x
#define STRINGIFY_LITERAL(x)    STRINGIFY(x)

#if __KERNEL__
#include <kernel/kernel.h>
#define tprint(...) kprint("test: " __VA_ARGS__)
#else
#error "Please define test macros for user mode!"
#endif

#define _GRN(s)     "\e[32m" s "\e[0m"
#define _YLW(s)     "\e[1;33m" s "\e[0m"
#define _RED(s)     "\e[31m" s "\e[0m"

#define DECLARE_TEST(description)                                               \
do {                                                                            \
    tprint("%s\n", description);                                                \
} while (0)

#define _FAIL_TEST(fn,msg,...)                                                  \
do {                                                                            \
    tprint("\n\e[1;30m" __FILE__ ":" STRINGIFY_LITERAL(__LINE__) ":");          \
    tprint("\n\e[1;31m*** TEST FAILED ***");                                    \
    tprint("\n\e[1;33m" fn "(" #__VA_ARGS__ ")");                               \
    tprint("\n\e[22;37m" msg, __VA_ARGS__);                                     \
    tprint("\e[0m");                                                            \
    for (;;);                                                                   \
} while (0)

#define _EMPTY_MSG      ""
#define _EXPACT_MSG     "EXPECTED: %d, ACTUAL: %d"
#define _VALUE_MSG      "VALUE: %d"

#define VERIFY_IS_TRUE(x)                                                       \
do {                                                                            \
    if (!(x)) {                                                                 \
        _FAIL_TEST("VERIFY_IS_TRUE", _EMPTY_MSG, x);                            \
    }                                                                           \
} while (0)

#define VERIFY_IS_FALSE(x)                                                      \
do {                                                                            \
    if (x) {                                                                    \
        _FAIL_TEST("VERIFY_IS_FALSE", _EMPTY_MSG, x);                           \
    }                                                                           \
} while (0)

#define VERIFY_IS_ZERO(x)                                                       \
do {                                                                            \
    if ((x) != 0) {                                                             \
        _FAIL_TEST("VERIFY_IS_ZERO", _VALUE_MSG, x);                            \
    }                                                                           \
} while (0)

#define VERIFY_IS_NOT_ZERO(x)                                                   \
do {                                                                            \
    if ((x) == 0) {                                                             \
        _FAIL_TEST("VERIFY_IS_NOT_ZERO", _VALUE_MSG, x);                        \
    }                                                                           \
} while (0)

#define VERIFY_IS_NULL(x)                                                       \
do {                                                                            \
    if ((x) != NULL) {                                                          \
        _FAIL_TEST("VERIFY_IS_NULL", _VALUE_MSG, x);                            \
    }                                                                           \
} while (0)

#define VERIFY_IS_NOT_NULL(x)                                                   \
do {                                                                            \
    if ((x) == NULL) {                                                          \
        _FAIL_TEST("VERIFY_IS_NOT_NULL", _VALUE_MSG, x);                        \
    }                                                                           \
} while (0)

#define VERIFY_ARE_EQUAL(expected,actual)                                       \
do {                                                                            \
    if ((actual) != (expected)) {                                               \
        _FAIL_TEST("VERIFY_ARE_EQUAL", _EXPACT_MSG,                             \
            expected, actual);                                                  \
    }                                                                           \
} while (0)

#define VERIFY_ARE_NOT_EQUAL(expected,actual)                                   \
do {                                                                            \
    if ((actual) == (expected)) {                                               \
        _FAIL_TEST("VERIFY_ARE_NOT_EQUAL", _EXPACT_MSG,                         \
            expected, actual);                                                  \
    }                                                                           \
} while (0)

#endif // __TEST_H
